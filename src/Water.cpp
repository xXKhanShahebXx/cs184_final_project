#include "Water.h"
#include <algorithm>

WaterGrid::WaterGrid(int nx, int nz, float dx, const Vec3& origin, float baseLevel)
    : nx(nx), nz(nz), dx(dx), origin(origin), baseLevel(baseLevel),
      h(nx * nz, baseLevel), u(nx * nz, 0.0f), v(nx * nz, 0.0f), q(nx * nz, 0.0f),
      hTmp(nx * nz, baseLevel), uTmp(nx * nz, 0.0f), vTmp(nx * nz, 0.0f), qTmp(nx * nz, 0.0f),
      gravity(9.81f), viscosity(0.05f), waveDamping(0.998f) {}

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

void WaterGrid::addRadialImpulse(float x, float z, float radius, float dh, float momentumScale) {
    int iCenter = std::clamp((int)((x - origin.x) / dx), 0, nx - 1);
    int kCenter = std::clamp((int)((z - origin.z) / dx), 0, nz - 1);
    int rCells = std::max(1, (int)(radius / dx));
    for (int dk = -rCells; dk <= rCells; ++dk) {
        for (int di = -rCells; di <= rCells; ++di) {
            int i = iCenter + di;
            int k = kCenter + dk;
            if (i < 1 || i >= nx - 1 || k < 1 || k >= nz - 1) continue;
            float dxw = di * dx;
            float dzw = dk * dx;
            float dist = std::sqrt(dxw * dxw + dzw * dzw);
            if (dist <= radius) {
                float sigma = radius * 0.5f;
                float w = std::exp(-(dist * dist) / (2.0f * sigma * sigma));
                float dhLocal = std::max(-0.004f, std::min(0.004f, dh)) * w;
                int id = idx(i, k);
                q[id] += dhLocal; // accumulate to source field for smooth injection
                float dirx = (dist > 1e-5f) ? (dxw / dist) : 0.0f;
                float dirz = (dist > 1e-5f) ? (dzw / dist) : 0.0f;
                u[id] += dirx * momentumScale * w * (dhLocal / std::max(radius, 1e-3f));
                v[id] += dirz * momentumScale * w * (dhLocal / std::max(radius, 1e-3f));
                float dev = h[id] - baseLevel;
                if (dev > 0.20f) h[id] = baseLevel + 0.20f;
                if (dev < -0.20f) h[id] = baseLevel - 0.20f;
            }
        }
    }
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

static void smoothHeights(std::vector<float>& h, std::vector<float>& tmp, int nx, int nz, float alpha) {
    for (int k = 1; k < nz - 1; ++k) {
        for (int i = 1; i < nx - 1; ++i) {
            int id = k * nx + i;
            float lap = h[id - 1] + h[id + 1] + h[id - nx] + h[id + nx] - 4.0f * h[id];
            tmp[id] = h[id] + alpha * lap;
        }
    }
    for (int k = 1; k < nz - 1; ++k) {
        for (int i = 1; i < nx - 1; ++i) {
            int id = k * nx + i;
            h[id] = tmp[id];
        }
    }
}

void WaterGrid::step(float dt) {
    float maxDt = 0.006f;
    int iters = std::max(1, (int)std::ceil(dt / maxDt));
    float hdt = dt / iters;
    for (int it = 0; it < iters; ++it) {
        diffuse(hdt);
        advect(hdt);
        project(hdt);
        for (int id = 0; id < nx * nz; ++id) {
            h[id] += q[id] * 0.9f; // inject smoothed source
            q[id] = 0.0f;
        }
        addHeightDamping(hdt);
        applyBoundary();
        float maxSpeed = 1.8f;
        for (int id = 0; id < nx * nz; ++id) {
            if (u[id] > maxSpeed) u[id] = maxSpeed; else if (u[id] < -maxSpeed) u[id] = -maxSpeed;
            if (v[id] > maxSpeed) v[id] = maxSpeed; else if (v[id] < -maxSpeed) v[id] = -maxSpeed;
        }
        smoothHeights(h, hTmp, nx, nz, 0.02f);
    }
}
