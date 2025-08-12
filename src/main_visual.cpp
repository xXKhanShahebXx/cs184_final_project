#include <GL/glew.h>
#include "Cloth.h"
#include "Water.h"
#include "Coupling.h"
#include "ClothRender.h"
#include "WaterRenderer.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <vector>
#include <GL/glut.h>

Cloth* cloth = nullptr;
float cameraDistance = 10.0f;
float cameraAngleX = 30.0f;
float cameraAngleY = 0.0f;

std::vector<unsigned char> textureData;
int textureWidth = 64;
int textureHeight = 64;

Vec3 windVelocity = Vec3(0.0f, 0.0f, 0.0f);
float windStrength = 0.0f;
float airDragCoefficient = 0.1f;
Vec3 windDir = Vec3(0.0f, 0.0f, 0.0f);

WaterGrid* water = nullptr;
CouplingParams coupling{ 400.0f, 2.0f, 1.0f };
WaterRenderer* waterRenderer = nullptr;

GLfloat light_position[] = { 1.0f, 10.0f, 1.0f, 1.0f };
GLfloat light_ambient[]  = { 0.9f, 0.9f, 0.9f, 1.0f };
GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };


void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    glTranslatef(0.0f, -2.0f, -cameraDistance);
    glRotatef(cameraAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraAngleY, 0.0f, 1.0f, 0.0f);

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    {
        Vec3 d = windDir;
        float len = length(d);
        if (len < 1e-4f) d = Vec3(1.0f, 0.0f, 0.0f); else d = d / len;
        float axisLen = 3.0f;
        Vec3 tip = d * axisLen;
        glDisable(GL_TEXTURE_2D);
        glLineWidth(3.0f);
        glColor3f(1.0f, 1.0f, 0.0f);
        glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(tip.x, tip.y, tip.z);
        glEnd();
        glLineWidth(1.0f);

        Vec3 side = Vec3(-d.z, 0.0f, d.x);
        float sLen = length(side);
        if (sLen < 1e-4f) side = Vec3(1.0f, 0.0f, 0.0f); else side = side / sLen;
        float headLen = 0.35f;
        float headWidth = 0.20f;
        Vec3 base = tip - d * headLen;
        Vec3 left = base + side * headWidth;
        Vec3 right = base - side * headWidth;
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 1.0f, 0.0f);
        glVertex3f(tip.x, tip.y, tip.z);
        glVertex3f(left.x, left.y, left.z);
        glVertex3f(right.x, right.y, right.z);
        glEnd();

        glRasterPos3f(tip.x + 0.1f, tip.y + 0.1f, tip.z + 0.1f);
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'W');
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    
    {
        auto& particles = cloth->getParticles();
        int gridW = static_cast<int>(std::sqrt(particles.size()));
        int gridH = gridW;
        
        glDisable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 1);
        GLfloat cloth_ambient[]   = { 0.3f, 0.3f, 0.3f, 1.0f };
        GLfloat cloth_diffuse[]   = { 0.7f, 0.7f, 0.7f, 1.0f }; 
        GLfloat cloth_specular[]  = { 0.1f, 0.1f, 0.1f, 1.0f }; 
        GLfloat cloth_shininess[] = { 8.0f };              
        
        glMaterialfv(GL_FRONT, GL_AMBIENT,   cloth_ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE,   cloth_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR,  cloth_specular);
        glMaterialfv(GL_FRONT, GL_SHININESS, cloth_shininess);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 1);
        // MODULATE combines the texture color with the light color
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor3f(1.0f, 1.0f, 1.0f);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());

        
        cloth->calculateNormals();
        
        glBegin(GL_TRIANGLES);
        for (int y = 0; y < gridH - 1; ++y) {
            for (int x = 0; x < gridW - 1; ++x) {
                const auto& p1 = particles[y * gridW + x];
                const auto& p2 = particles[y * gridW + x + 1];
                const auto& p3 = particles[(y + 1) * gridW + x + 1];
                const auto& p4 = particles[(y + 1) * gridW + x];
                
                float u1 = static_cast<float>(x) / (gridW - 1);
                float v1 = static_cast<float>(y) / (gridH - 1);
                float u2 = static_cast<float>(x + 1) / (gridW - 1);
                float v2 = static_cast<float>(y + 1) / (gridH - 1);

                // Triangle 1 (p1, p2, p3)
                glNormal3f(p1.normal.x, p1.normal.y, p1.normal.z);
                glTexCoord2f(u1, v1); glVertex3f(p1.position.x, p1.position.y, p1.position.z);
                
                glNormal3f(p2.normal.x, p2.normal.y, p2.normal.z);
                glTexCoord2f(u2, v1); glVertex3f(p2.position.x, p2.position.y, p2.position.z);
                
                glNormal3f(p3.normal.x, p3.normal.y, p3.normal.z);
                glTexCoord2f(u2, v2); glVertex3f(p3.position.x, p3.position.y, p3.position.z);

                // Triangle 2 (p1, p3, p4)
                glNormal3f(p1.normal.x, p1.normal.y, p1.normal.z);
                glTexCoord2f(u1, v1); glVertex3f(p1.position.x, p1.position.y, p1.position.z);

                glNormal3f(p3.normal.x, p3.normal.y, p3.normal.z);
                glTexCoord2f(u2, v2); glVertex3f(p3.position.x, p3.position.y, p3.position.z);

                glNormal3f(p4.normal.x, p4.normal.y, p4.normal.z);
                glTexCoord2f(u1, v2); glVertex3f(p4.position.x, p4.position.y, p4.position.z);
            }
        }   
                glEnd();
                glDisable(GL_TEXTURE_2D);
    }

    if (waterRenderer && water) {
        waterRenderer->updateFromWater(*water);
        float view[16];
        float proj[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, view);
        glGetFloatv(GL_PROJECTION_MATRIX, proj);
        waterRenderer->draw(view, proj);
    }

    glBegin(GL_LINES);
    glColor3f(0.8f, 0.8f, 0.8f);
    
    const auto& springs = cloth->getSprings();
    const auto& particles = cloth->getParticles();
    
    for (const auto& spring : springs) {
        const auto& p1 = particles[spring.particle1];
        const auto& p2 = particles[spring.particle2];
        
        glVertex3f(p1.position.x, p1.position.y, p1.position.z);
        glVertex3f(p2.position.x, p2.position.y, p2.position.z);
    }
    glEnd();
    
    glBegin(GL_POINTS);
    glColor3f(1.0f, 0.0f, 0.0f);
    glPointSize(3.0f);
    
    for (const auto& particle : particles) {
        if (particle.fixed) {
            glColor3f(0.0f, 1.0f, 0.0f);
        } else {
            glColor3f(1.0f, 0.0f, 0.0f);
        }
        glVertex3f(particle.position.x, particle.position.y, particle.position.z);
    }
    glEnd();

    glutSwapBuffers();
}

void generateTexture() {
    textureData.resize(textureWidth * textureHeight * 3);
    
    for (int y = 0; y < textureHeight; ++y) {
        for (int x = 0; x < textureWidth; ++x) {
            int index = (y * textureWidth + x) * 3;
            
            bool checker = ((x / 6) + (y / 6)) % 2 == 0;
            if (checker) {
                textureData[index]     = 240;
                textureData[index + 1] = 140;
                textureData[index + 2] = 0;
            } else {
                textureData[index]     = 245;
                textureData[index + 1] = 245;
                textureData[index + 2] = 245;
            }
        }
    }
}

void update() {
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    
    deltaTime = std::min(deltaTime, 0.016f);
    
    Vec3 gravity(0.0f, -2.0f, 0.0f);
    {
        float len = length(windDir);
        Vec3 dir = (len > 1e-4f) ? (windDir / len) : Vec3(1.0f, 0.0f, 0.0f);
        windVelocity = dir * windStrength;
    }
    bool windOn = (windStrength > 0.0f);
    Vec3 airVel = windOn ? windVelocity : Vec3(0.0f, 0.0f, 0.0f);
    cloth->prepareForces();
    applyWaterToCloth(*water, *cloth, coupling);
    cloth->applyGravity(gravity);
    cloth->applyAirDrag(airDragCoefficient, airVel);
    {
        auto& pts = cloth->getParticles();
        for (auto& p : pts) {
            if (!p.fixed) {
                p.velocity.x += airVel.x * 0.03f * deltaTime;
                p.velocity.z += airVel.z * 0.03f * deltaTime;
            }
        }
    }
    cloth->finalizeIntegration(deltaTime);
    applyClothToWater(*water, *cloth, coupling, deltaTime);
    water->step(deltaTime);

    if (windOn) {
        static int frameCount = 0;
        if (frameCount++ % 60 == 0) {
            std::cout << "Wind active: " << airVel.x << ", " << airVel.y << ", " << airVel.z << std::endl;
        }
    }
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27:
            exit(0);
            break;
        case 'w':
            cameraAngleX += 5.0f;
            break;
        case 's':
            cameraAngleX -= 5.0f;
            break;
        case 'a':
            cameraAngleY -= 5.0f;
            break;
        case 'd':
            cameraAngleY += 5.0f;
            break;
        case '+':
        case '=':
            cameraDistance -= 0.5f;
            break;
        case '-':
            cameraDistance += 0.5f;
            break;
        case '1': windDir = Vec3(1.0f, 0.0f, 0.0f); break;
        case '2': windDir = Vec3(0.0f, 1.0f, 0.0f); break;
        case '3': windDir = Vec3(0.0f, 0.0f, 1.0f); break;
        case '7': windDir = Vec3(-1.0f, 0.0f, 0.0f); break;
        case '8': windDir = Vec3(0.0f, -1.0f, 0.0f); break;
        case '9': windDir = Vec3(0.0f, 0.0f, -1.0f); break;
        case 'j': windDir.x -= 0.1f; break;
        case 'l': windDir.x += 0.1f; break;
        case 'i': windDir.z += 0.1f; break;
        case 'k': windDir.z -= 0.1f; break;
        case 'u': windDir.y += 0.1f; break;
        case 'o': windDir.y -= 0.1f; break;
        case 'c': windStrength = 0.0f; break;
        case '4': windStrength = std::min(20.0f, windStrength + 1.0f); if (length(windDir) <= 1e-4f) windDir = Vec3(1.0f,0.0f,0.0f); break;
        case '5': windStrength = std::max(0.0f, windStrength - 1.0f); break;
        case 'r':
            delete cloth;
            cloth = new Cloth(15, 15, 0.15f);
            cloth->fixCorner(0);
            windDir = Vec3(0.0f, 0.0f, 0.0f);
            windStrength = 0.0f;
            std::cout << "Cloth reset to original position with fixed corner" << std::endl;
            break;
    }
    glutPostRedisplay();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w / (float)h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    std::cout << "Cloth Simulation - Stage 3 (Wind Effects)" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  W/S - Rotate camera up/down" << std::endl;
    std::cout << "  A/D - Rotate camera left/right" << std::endl;
    std::cout << "  +/- - Zoom in/out" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "  Wind Controls:" << std::endl;
    std::cout << "    1/2/3 - Wind +X/+Y/+Z" << std::endl;
    std::cout << "    7/8/9 - Wind -X/-Y/-Z" << std::endl;
    std::cout << "    4/5 - Increase/decrease wind strength (wind active only if strength > 0)" << std::endl;
    std::cout << "    C - Clear wind (strength = 0)" << std::endl;
    std::cout << "    R - Reset cloth" << std::endl;
    std::cout << std::endl;
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Cloth Simulation - Stage 3");
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glEnable(GL_NORMALIZE);

    
    generateTexture();
    
    cloth = new Cloth(15, 15, 0.15f);
    cloth->fixCorner(0);
    water = new WaterGrid(80, 80, 0.12f, Vec3(-4.0f, -1.3f, -4.0f), -0.8f);
    waterRenderer = new WaterRenderer(80, 80, 0.12f, Vec3(-4.0f, -1.3f, -4.0f), -0.8f, -1.4f);
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(update);
    
    std::cout << "Created cloth with " << cloth->getParticles().size() << " particles" << std::endl;
    std::cout << "Created " << cloth->getSprings().size() << " springs" << std::endl;
    std::cout << "Window opened - you should see the cloth falling!" << std::endl;
    std::cout << "Initial wind strength: " << windStrength << std::endl;
    std::cout << "Coordinate System:" << std::endl;
    std::cout << "  Red axis (X) = Left/Right" << std::endl;
    std::cout << "  Green axis (Y) = Up/Down" << std::endl;
    std::cout << "  Blue axis (Z) = Forward/Backward" << std::endl;
    std::cout << "  Yellow arrow (W) = Wind direction" << std::endl;
    std::cout << "Press 1/2/3 to set wind direction. Wind applies only when strength > 0 (press 4)." << std::endl;
    
    glutMainLoop();
    
    delete cloth;
    delete waterRenderer;
    delete water;
    return 0;
} 