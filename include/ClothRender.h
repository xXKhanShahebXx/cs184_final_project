#pragma once
#include <GL/glut.h>
#include "Water.h"

inline void drawWaterSurface(const WaterGrid& water) {
    int nx = water.getNx();
    int nz = water.getNz();
    float dx = water.getDx();
    Vec3 org = water.getOrigin();

    const float base = water.getBaseLevel();
    const float poolDepth = 0.6f;
    const float bottomY = base - poolDepth;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.05f, 0.15f, 0.35f, 0.9f);
    glBegin(GL_QUADS);
    glVertex3f(org.x,                 bottomY, org.z);
    glVertex3f(org.x + (nx - 1) * dx, bottomY, org.z);
    glVertex3f(org.x + (nx - 1) * dx, bottomY, org.z + (nz - 1) * dx);
    glVertex3f(org.x,                 bottomY, org.z + (nz - 1) * dx);
    glEnd();

    glBegin(GL_QUADS);
    for (int k = 0; k < nz - 1; ++k) {
        for (int i = 0; i < nx - 1; ++i) {
            float x0 = org.x + i * dx;
            float z0 = org.z + k * dx;
            float x1 = org.x + (i + 1) * dx;
            float z1 = org.z + (k + 1) * dx;
            float h00 = water.getH()[k * nx + i];
            float h10 = water.getH()[k * nx + (i + 1)];
            float h11 = water.getH()[(k + 1) * nx + (i + 1)];
            float h01 = water.getH()[(k + 1) * nx + i];

            auto shade = [&](float h) {
                float t = (h - base) * 1.2f;
                float r = 0.10f + 0.10f * t;
                float g = 0.40f + 0.20f * t;
                float b = 0.80f + 0.10f * t;
                glColor4f(r, g, b, 0.65f);
            };

            shade(h00); glVertex3f(x0, h00, z0);
            shade(h10); glVertex3f(x1, h10, z0);
            shade(h11); glVertex3f(x1, h11, z1);
            shade(h01); glVertex3f(x0, h01, z1);
        }
    }
    glEnd();

    glColor4f(0.06f, 0.18f, 0.40f, 0.75f);
    glBegin(GL_QUADS);
    for (int k = 0; k < nz - 1; ++k) {
        float x = org.x;
        float z0 = org.z + k * dx;
        float z1 = org.z + (k + 1) * dx;
        glVertex3f(x, bottomY, z0);
        glVertex3f(x, base,    z0);
        glVertex3f(x, base,    z1);
        glVertex3f(x, bottomY, z1);
    }
    for (int k = 0; k < nz - 1; ++k) {
        float x = org.x + (nx - 1) * dx;
        float z0 = org.z + k * dx;
        float z1 = org.z + (k + 1) * dx;
        glVertex3f(x, bottomY, z0);
        glVertex3f(x, base,    z0);
        glVertex3f(x, base,    z1);
        glVertex3f(x, bottomY, z1);
    }
    for (int i = 0; i < nx - 1; ++i) {
        float z = org.z;
        float x0 = org.x + i * dx;
        float x1 = org.x + (i + 1) * dx;
        glVertex3f(x0, bottomY, z);
        glVertex3f(x0, base,    z);
        glVertex3f(x1, base,    z);
        glVertex3f(x1, bottomY, z);
    }
    for (int i = 0; i < nx - 1; ++i) {
        float z = org.z + (nz - 1) * dx;
        float x0 = org.x + i * dx;
        float x1 = org.x + (i + 1) * dx;
        glVertex3f(x0, bottomY, z);
        glVertex3f(x0, base,    z);
        glVertex3f(x1, base,    z);
        glVertex3f(x1, bottomY, z);
    }
    glEnd();

    glDisable(GL_BLEND);
}
