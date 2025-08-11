#pragma once
#include <GL/glut.h>
#include <cmath>
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

            auto normalAt = [&](float x, float z, float hL, float hR, float hD, float hU) {
                float dhdx = (hR - hL) / (2.0f * dx);
                float dhdz = (hU - hD) / (2.0f * dx);
                Vec3 n = Vec3(-dhdx, 1.0f, -dhdz).normalize();
                return n;
            };

            auto fresnelShade = [&](float x, float z, float h, int ii, int kk) {
                int il = std::max(0, ii - 1), ir = std::min(nx - 1, ii + 1);
                int kd = std::max(0, kk - 1), ku = std::min(nz - 1, kk + 1);
                float hL = water.getH()[kk * nx + il];
                float hR = water.getH()[kk * nx + ir];
                float hD = water.getH()[kd * nx + ii];
                float hU = water.getH()[ku * nx + ii];
                Vec3 n = normalAt(x, z, hL, hR, hD, hU);
                Vec3 viewDir = Vec3(0.0f, 0.8f, 0.6f).normalize();
                float cosTheta = std::max(0.0f, std::min(1.0f, n.dot(viewDir)));
                float F0 = 0.02f;
                float F = F0 + (1.0f - F0) * std::pow(1.0f - cosTheta, 5.0f);
                float tSky = std::max(0.0f, n.y);
                float rR = 0.35f * (1.0f - tSky) + 0.60f * tSky;
                float rG = 0.45f * (1.0f - tSky) + 0.70f * tSky;
                float rB = 0.70f * (1.0f - tSky) + 0.95f * tSky;
                float depthT = std::max(0.0f, std::min(1.0f, (h - bottomY) / (base - bottomY + 1e-4f)));
                float tR = 0.04f + 0.08f * depthT;
                float tG = 0.25f + 0.20f * depthT;
                float tB = 0.35f + 0.25f * depthT;
                float outR = tR * (1.0f - F) + rR * F;
                float outG = tG * (1.0f - F) + rG * F;
                float outB = tB * (1.0f - F) + rB * F;
                Vec3 lightDir = Vec3(0.3f, 0.9f, 0.3f).normalize();
                Vec3 halfV = (lightDir + viewDir).normalize();
                float spec = std::pow(std::max(0.0f, n.dot(halfV)), 64.0f) * 0.35f;
                float dhdx = (hR - hL) / (2.0f * dx);
                float dhdz = (hU - hD) / (2.0f * dx);
                float slope = std::sqrt(dhdx * dhdx + dhdz * dhdz);
                Vec3 vel = water.sampleVelocity(x, z);
                float speed = std::sqrt(vel.x * vel.x + vel.z * vel.z);
                float foam = std::max(0.0f, std::min(1.0f, (slope * 3.0f + speed * 0.7f - 0.35f) * 1.4f));
                outR = outR * (1.0f - foam) + 1.0f * foam;
                outG = outG * (1.0f - foam) + 1.0f * foam;
                outB = outB * (1.0f - foam) + 1.0f * foam;
                glColor4f(std::min(1.0f, outR + spec), std::min(1.0f, outG + spec), std::min(1.0f, outB + spec), 0.75f);
            };

            fresnelShade(x0, z0, h00, i, k); glVertex3f(x0, h00, z0);
            fresnelShade(x1, z0, h10, i + 1, k); glVertex3f(x1, h10, z0);
            fresnelShade(x1, z1, h11, i + 1, k + 1); glVertex3f(x1, h11, z1);
            fresnelShade(x0, z1, h01, i, k + 1); glVertex3f(x0, h01, z1);
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
