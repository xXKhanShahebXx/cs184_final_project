#pragma once
#include <vector>
#include <GL/glew.h>
#include "SimpleMath.h"
#include "Water.h"

class WaterRenderer {
public:
    WaterRenderer(int nx, int nz, float dx, const Vec3& origin, float baseLevel, float bottomLevel);
    ~WaterRenderer();

    void updateFromWater(const WaterGrid& water);
    void draw(const float* view, const float* proj);

private:
    int nx, nz;
    float dx;
    Vec3 origin;
    float baseLevel;
    float bottomLevel;

    GLuint vao;
    GLuint vbo;
    GLuint nbo;
    GLuint ibo;
    GLuint program;

    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<unsigned int> indices;

    GLint uView;
    GLint uProj;
    GLint uModel;
    GLint uEye;
    GLint uLightDir;
    GLint uBase;
    GLint uBottom;

    void buildMesh();
    void computeNormals(const WaterGrid& water);
    GLuint compile(GLenum type, const char* src);
    GLuint link(GLuint vs, GLuint fs);
};


