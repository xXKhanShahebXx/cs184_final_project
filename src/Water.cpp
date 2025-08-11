#include "Water.h"
#include <algorithm>

WaterGrid::WaterGrid(int nx, int nz, float dx, const Vec3& origin, float baseLevel)
    : nx(nx), nz(nz), dx(dx), origin(origin), baseLevel(baseLevel),
      h(nx * nz, baseLevel), u(nx * nz, 0.0f), v(nx * nz, 0.0f),
      hTmp(nx * nz, baseLevel), uTmp(nx * nz, 0.0f), vTmp(nx * nz, 0.0f),
      gravity(9.81f), viscosity(0.05f) {}

float WaterGrid::sampleHeight(float x, float z) const {
    float fx = (x - origin.x) / dx;
    float fz = (z - origin.z) / dx;
    int i = std::clamp((int)fx, 0, nx - 1);
    int k = std::clamp((int)fz, 0, nz - 1);
    return h[idx(i, k)];
}

Vec3 WaterGrid::sampleVelocity(float x, float z) const {
    float fx = (x - origin.x) / dx;
    float fz = (z - origin.z) / dx;
    int i = std::clamp((int)fx, 0, nx - 1);
    int k = std::clamp((int)fz, 0, nz - 1);
    return Vec3(u[idx(i, k)], 0.0f, v[idx(i, k)]);
}

void WaterGrid::addImpulse(float x, float z, float du, float dv, float dh) {
    float fx = (x - origin.x) / dx;
    float fz = (z - origin.z) / dx;
    int i = std::clamp((int)fx, 0, nx - 1);
    int k = std::clamp((int)fz, 0, nz - 1);
    int id = idx(i, k);
    u[id] += du;
    v[id] += dv;
    h[id] += dh;
}

void WaterGrid::applyBoundary() {
    for (int i = 0; i < nx; ++i) {
        u[idx(i, 0)] = 0.0f; v[idx(i, 0)] = 0.0f; h[idx(i, 0)] = std::max(h[idx(i, 0)], baseLevel);
        u[idx(i, nz - 1)] = 0.0f; v[idx(i, nz - 1)] = 0.0f; h[idx(i, nz - 1)] = std::max(h[idx(i, nz - 1)], baseLevel);
    }
    for (int k = 0; k < nz; ++k) {
        u[idx(0, k)] = 0.0f; v[idx(0, k)] = 0.0f; h[idx(0, k)] = std::max(h[idx(0, k)], baseLevel);
        u[idx(nx - 1, k)] = 0.0f; v[idx(nx - 1, k)] = 0.0f; h[idx(nx - 1, k)] = std::max(h[idx(nx - 1, k)], baseLevel);
    }
}

void WaterGrid::diffuse(float dt) {
    float a = viscosity * dt / (dx * dx);
    for (int it = 0; it < 10; ++it) {
        for (int k = 1; k < nz - 1; ++k) {
            for (int i = 1; i < nx - 1; ++i) {
                int id = idx(i, k);
                uTmp[id] = (u[id] + a * (u[idx(i + 1, k)] + u[idx(i - 1, k)] + u[idx(i, k + 1)] + u[idx(i, k - 1)])) / (1.0f + 4.0f * a);
                vTmp[id] = (v[id] + a * (v[idx(i + 1, k)] + v[idx(i - 1, k)] + v[idx(i, k + 1)] + v[idx(i, k - 1)])) / (1.0f + 4.0f * a);
            }
        }
        std::swap(u, uTmp);
        std::swap(v, vTmp);
    }
}

void WaterGrid::advect(float dt) {
    for (int k = 1; k < nz - 1; ++k) {
        for (int i = 1; i < nx - 1; ++i) {
            int id = idx(i, k);
            float x = i - u[id] * dt / dx;
            float z = k - v[id] * dt / dx;
            x = std::clamp(x, 1.0f, (float)nx - 2);
            z = std::clamp(z, 1.0f, (float)nz - 2);
            int i0 = (int)x, k0 = (int)z;
            uTmp[id] = u[idx(i0, k0)];
            vTmp[id] = v[idx(i0, k0)];
            hTmp[id] = h[idx(i0, k0)];
        }
    }
    std::swap(u, uTmp);
    std::swap(v, vTmp);
    std::swap(h, hTmp);
}

void WaterGrid::project(float dt) {
    for (int k = 1; k < nz - 1; ++k) {
        for (int i = 1; i < nx - 1; ++i) {
            int id = idx(i, k);
            float dhdx = (h[idx(i + 1, k)] - h[idx(i - 1, k)]) / (2.0f * dx);
            float dhdz = (h[idx(i, k + 1)] - h[idx(i, k - 1)]) / (2.0f * dx);
            u[id] += -gravity * dhdx * dt;
            v[id] += -gravity * dhdz * dt;
        }
    }
}

void WaterGrid::addHeightDamping(float dt) {
    float damp = 0.998f;
    for (int id = 0; id < nx * nz; ++id) {
        h[id] = baseLevel + (h[id] - baseLevel) * damp;
    }
}

void WaterGrid::step(float dt) {
    diffuse(dt);
    advect(dt);
    project(dt);
    addHeightDamping(dt);
    applyBoundary();
}
