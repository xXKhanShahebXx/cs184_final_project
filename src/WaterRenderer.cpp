#include "WaterRenderer.h"
#include <cmath>
#include <cstring>

static const char* kWaterVS = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;

uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uModel;

out vec3 vPosWS;
out vec3 vNormalWS;

void main(){
    vec4 ws = uModel * vec4(aPos,1.0);
    vPosWS = ws.xyz;
    vNormalWS = mat3(uModel) * aNormal;
    gl_Position = uProj * uView * ws;
}
)";

static const char* kWaterFS = R"(
#version 330 core
in vec3 vPosWS;
in vec3 vNormalWS;
out vec4 FragColor;

uniform vec3 uEye;
uniform vec3 uLightDir;
uniform float uBase;
uniform float uBottom;

vec3 skyColor(vec3 n){
    float t = clamp(n.y*0.5+0.5,0.0,1.0);
    return mix(vec3(0.15,0.25,0.45), vec3(0.55,0.75,1.0), t);
}

vec3 waterTransmittance(float y){
    float depth = clamp((y - uBottom)/(uBase-uBottom+1e-4),0.0,1.0);
    return mix(vec3(0.02,0.18,0.35), vec3(0.15,0.35,0.55), depth);
}

void main(){
    vec3 N = normalize(vNormalWS);
    vec3 V = normalize(uEye - vPosWS);
    vec3 L = normalize(uLightDir);
    float NdotV = clamp(dot(N,V),0.0,1.0);
    float F0 = 0.02;
    float Fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);

    vec3 Rcol = skyColor(N);
    vec3 Tcol = waterTransmittance(vPosWS.y);

    vec3 H = normalize(V+L);
    float spec = pow(max(dot(N,H),0.0), 96.0) * 0.35;

    float foam = clamp((1.0-abs(N.y))*2.0,0.0,1.0);

    vec3 baseCol = mix(Tcol, Rcol, Fresnel);
    baseCol += spec;
    baseCol = mix(baseCol, vec3(1.0), foam*0.4);

    FragColor = vec4(baseCol, 0.85);
}
)";

static const char* kErr = "Shader compile/link error";

WaterRenderer::WaterRenderer(int nx, int nz, float dx, const Vec3& origin, float baseLevel, float bottomLevel)
    : nx(nx), nz(nz), dx(dx), origin(origin), baseLevel(baseLevel), bottomLevel(bottomLevel),
      vao(0), vbo(0), nbo(0), ibo(0), program(0) {
    buildMesh();
    GLuint vs = compile(GL_VERTEX_SHADER, kWaterVS);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kWaterFS);
    program = link(vs, fs);
    glDeleteShader(vs); glDeleteShader(fs);

    uView   = glGetUniformLocation(program, "uView");
    uProj   = glGetUniformLocation(program, "uProj");
    uModel  = glGetUniformLocation(program, "uModel");
    uEye    = glGetUniformLocation(program, "uEye");
    uLightDir = glGetUniformLocation(program, "uLightDir");
    uBase   = glGetUniformLocation(program, "uBase");
    uBottom = glGetUniformLocation(program, "uBottom");
}

WaterRenderer::~WaterRenderer(){
    if(ibo) glDeleteBuffers(1,&ibo);
    if(nbo) glDeleteBuffers(1,&nbo);
    if(vbo) glDeleteBuffers(1,&vbo);
    if(vao) glDeleteVertexArrays(1,&vao);
    if(program) glDeleteProgram(program);
}

void WaterRenderer::buildMesh(){
    positions.resize(nx*nz);
    normals.resize(nx*nz, Vec3(0,1,0));
    indices.clear(); indices.reserve((nx-1)*(nz-1)*6);
    for(int k=0;k<nz;++k){
        for(int i=0;i<nx;++i){
            positions[k*nx+i] = Vec3(origin.x + i*dx, baseLevel, origin.z + k*dx);
        }
    }
    for(int k=0;k<nz-1;++k){
        for(int i=0;i<nx-1;++i){
            int i0 = k*nx + i;
            int i1 = k*nx + i+1;
            int i2 = (k+1)*nx + i+1;
            int i3 = (k+1)*nx + i;
            indices.push_back(i0); indices.push_back(i1); indices.push_back(i2);
            indices.push_back(i0); indices.push_back(i2); indices.push_back(i3);
        }
    }

    glGenVertexArrays(1,&vao); glBindVertexArray(vao);
    glGenBuffers(1,&vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, positions.size()*sizeof(Vec3), positions.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vec3),(void*)0);

    glGenBuffers(1,&nbo); glBindBuffer(GL_ARRAY_BUFFER, nbo);
    glBufferData(GL_ARRAY_BUFFER, normals.size()*sizeof(Vec3), normals.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vec3),(void*)0);

    glGenBuffers(1,&ibo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void WaterRenderer::computeNormals(const WaterGrid& water){
    for(int k=0;k<nz;++k){
        for(int i=0;i<nx;++i){
            int il = std::max(0,i-1), ir = std::min(nx-1,i+1);
            int kd = std::max(0,k-1), ku = std::min(nz-1,k+1);
            float hL = water.getH()[k*nx+il];
            float hR = water.getH()[k*nx+ir];
            float hD = water.getH()[kd*nx+i];
            float hU = water.getH()[ku*nx+i];
            float dhdx = (hR - hL)/(2.0f*dx);
            float dhdz = (hU - hD)/(2.0f*dx);
            Vec3 n = Vec3(-dhdx, 1.0f, -dhdz).normalize();
            normals[k*nx+i] = n;
        }
    }
}

void WaterRenderer::updateFromWater(const WaterGrid& water){
    for(int k=0;k<nz;++k){
        for(int i=0;i<nx;++i){
            positions[k*nx+i].y = water.getH()[k*nx+i];
        }
    }
    computeNormals(water);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size()*sizeof(Vec3), positions.data());
    glBindBuffer(GL_ARRAY_BUFFER, nbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, normals.size()*sizeof(Vec3), normals.data());
}

void WaterRenderer::draw(const float* view, const float* proj){
    auto inv4 = [](const float m[16], float invOut[16]){
        float inv[16];
        inv[0] = m[5]  * m[10] * m[15] - 
                 m[5]  * m[11] * m[14] - 
                 m[9]  * m[6]  * m[15] + 
                 m[9]  * m[7]  * m[14] + 
                 m[13] * m[6]  * m[11] - 
                 m[13] * m[7]  * m[10];

        inv[4] = -m[4]  * m[10] * m[15] + 
                  m[4]  * m[11] * m[14] + 
                  m[8]  * m[6]  * m[15] - 
                  m[8]  * m[7]  * m[14] - 
                  m[12] * m[6]  * m[11] + 
                  m[12] * m[7]  * m[10];

        inv[8] = m[4]  * m[9] * m[15] - 
                 m[4]  * m[11] * m[13] - 
                 m[8]  * m[5] * m[15] + 
                 m[8]  * m[7] * m[13] + 
                 m[12] * m[5] * m[11] - 
                 m[12] * m[7] * m[9];

        inv[12] = -m[4]  * m[9] * m[14] + 
                   m[4]  * m[10] * m[13] + 
                   m[8]  * m[5] * m[14] - 
                   m[8]  * m[6] * m[13] - 
                   m[12] * m[5] * m[10] + 
                   m[12] * m[6] * m[9];

        inv[1] = -m[1]  * m[10] * m[15] + 
                  m[1]  * m[11] * m[14] + 
                  m[9]  * m[2] * m[15] - 
                  m[9]  * m[3] * m[14] - 
                  m[13] * m[2] * m[11] + 
                  m[13] * m[3] * m[10];

        inv[5] = m[0]  * m[10] * m[15] - 
                 m[0]  * m[11] * m[14] - 
                 m[8]  * m[2] * m[15] + 
                 m[8]  * m[3] * m[14] + 
                 m[12] * m[2] * m[11] - 
                 m[12] * m[3] * m[10];

        inv[9] = -m[0]  * m[9] * m[15] + 
                  m[0]  * m[11] * m[13] + 
                  m[8]  * m[1] * m[15] - 
                  m[8]  * m[3] * m[13] - 
                  m[12] * m[1] * m[11] + 
                  m[12] * m[3] * m[9];

        inv[13] = m[0]  * m[9] * m[14] - 
                  m[0]  * m[10] * m[13] - 
                  m[8]  * m[1] * m[14] + 
                  m[8]  * m[2] * m[13] + 
                  m[12] * m[1] * m[10] - 
                  m[12] * m[2] * m[9];

        inv[2] = m[1]  * m[6] * m[15] - 
                 m[1]  * m[7] * m[14] - 
                 m[5]  * m[2] * m[15] + 
                 m[5]  * m[3] * m[14] + 
                 m[13] * m[2] * m[7] - 
                 m[13] * m[3] * m[6];

        inv[6] = -m[0]  * m[6] * m[15] + 
                  m[0]  * m[7] * m[14] + 
                  m[4]  * m[2] * m[15] - 
                  m[4]  * m[3] * m[14] - 
                  m[12] * m[2] * m[7] + 
                  m[12] * m[3] * m[6];

        inv[10] = m[0]  * m[5] * m[15] - 
                  m[0]  * m[7] * m[13] - 
                  m[4]  * m[1] * m[15] + 
                  m[4]  * m[3] * m[13] + 
                  m[12] * m[1] * m[7] - 
                  m[12] * m[3] * m[5];

        inv[14] = -m[0]  * m[5] * m[14] + 
                   m[0]  * m[6] * m[13] + 
                   m[4]  * m[1] * m[14] - 
                   m[4]  * m[2] * m[13] - 
                   m[12] * m[1] * m[6] + 
                   m[12] * m[2] * m[5];

        inv[3] = -m[1] * m[6] * m[11] + 
                  m[1] * m[7] * m[10] + 
                  m[5] * m[2] * m[11] - 
                  m[5] * m[3] * m[10] - 
                  m[9] * m[2] * m[7] + 
                  m[9] * m[3] * m[6];

        inv[7] = m[0] * m[6] * m[11] - 
                 m[0] * m[7] * m[10] - 
                 m[4] * m[2] * m[11] + 
                 m[4] * m[3] * m[10] + 
                 m[8] * m[2] * m[7] - 
                 m[8] * m[3] * m[6];

        inv[11] = -m[0] * m[5] * m[11] + 
                   m[0] * m[7] * m[9] + 
                   m[4] * m[1] * m[11] - 
                   m[4] * m[3] * m[9] - 
                   m[8] * m[1] * m[7] + 
                   m[8] * m[3] * m[5];

        inv[15] = m[0] * m[5] * m[10] - 
                  m[0] * m[6] * m[9] - 
                  m[4] * m[1] * m[10] + 
                  m[4] * m[2] * m[9] + 
                  m[8] * m[1] * m[6] - 
                  m[8] * m[2] * m[5];

        float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
        det = det == 0.0f ? 1.0f : det;
        det = 1.0f / det;
        for (int i = 0; i < 16; i++) invOut[i] = inv[i] * det;
    };

    glUseProgram(program);
    glUniformMatrix4fv(uView,1,GL_FALSE,view);
    glUniformMatrix4fv(uProj,1,GL_FALSE,proj);
    float model[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    glUniformMatrix4fv(uModel,1,GL_FALSE,model);
    float invV[16]; inv4(view, invV);
    GLfloat eye[3] = {invV[12], invV[13], invV[14]};
    glUniform3fv(uEye,1,eye);
    GLfloat lightDir[3] = {0.3f,0.9f,0.3f};
    glUniform3fv(uLightDir,1,lightDir);
    glUniform1f(uBase, baseLevel);
    glUniform1f(uBottom, bottomLevel);

    glBindVertexArray(vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glUseProgram(0);
}

GLuint WaterRenderer::compile(GLenum type, const char* src){
    GLuint sh = glCreateShader(type);
    glShaderSource(sh,1,&src,nullptr);
    glCompileShader(sh);
    GLint ok=0; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if(!ok){
        char log[1024]; GLsizei len=0; glGetShaderInfoLog(sh,1024,&len,log);
        glDeleteShader(sh); throw kErr;
    }
    return sh;
}

GLuint WaterRenderer::link(GLuint vs, GLuint fs){
    GLuint prog = glCreateProgram();
    glAttachShader(prog,vs); glAttachShader(prog,fs);
    glBindAttribLocation(prog,0,"aPos");
    glBindAttribLocation(prog,1,"aNormal");
    glLinkProgram(prog);
    GLint ok=0; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if(!ok){
        char log[1024]; GLsizei len=0; glGetProgramInfoLog(prog,1024,&len,log);
        glDeleteProgram(prog); throw kErr;
    }
    return prog;
}


