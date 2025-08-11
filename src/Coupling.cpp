#include "Coupling.h"
#include <algorithm>

static float clampf(float x, float a, float b) { return std::max(a, std::min(b, x)); }

void applyWaterToCloth(const WaterGrid& water, Cloth& cloth, const CouplingParams& p) {
    auto& particles = const_cast<std::vector<Particle>&>(cloth.getParticles());
    for (auto& particle : particles) {
        if (particle.fixed) continue;
        float waterH = water.sampleHeight(particle.position.x, particle.position.z);
        float depth = std::max(0.0f, waterH - particle.position.y);
        if (depth > 0.0f) {
            Vec3 pressureForce(0.0f, p.pressureCoeff * depth, 0.0f);
            Vec3 waterVel = water.sampleVelocity(particle.position.x, particle.position.z);
            Vec3 relVel = Vec3(particle.velocity.x - waterVel.x, 0.0f, particle.velocity.z - waterVel.z);
            float scale = std::min(1.0f, depth / 0.25f);
            Vec3 dragForce = -relVel * (p.dragCoeff * scale);
            particle.force += pressureForce + dragForce;
        }
    }
}

void applyClothToWater(WaterGrid& water, const Cloth& cloth, const CouplingParams& p, float dt) {
    const auto& particles = cloth.getParticles();
    const auto& H = water.getH();
    const int nx = water.getNx();
    const int nz = water.getNz();
    const float dx = water.getDx();
    const Vec3 org = water.getOrigin();

    for (const auto& particle : particles) {
        float x = particle.position.x;
        float z = particle.position.z;
        float waterH = water.sampleHeight(x, z);
        float depth = std::max(0.0f, waterH - particle.position.y);

        float fx = (x - org.x) / dx;
        float fz = (z - org.z) / dx;
        int i = std::max(1, std::min(nx - 2, (int)fx));
        int k = std::max(1, std::min(nz - 2, (int)fz));
        auto idx = [&](int ii, int kk){ return kk * nx + ii; };
        float hL = H[idx(i - 1, k)], hR = H[idx(i + 1, k)];
        float hD = H[idx(i, k - 1)], hU = H[idx(i, k + 1)];
        float dhdx = (hR - hL) / (2.0f * dx);
        float dhdz = (hU - hD) / (2.0f * dx);

        float vx = particle.velocity.x;
        float vz = particle.velocity.z;
        float vy = particle.velocity.y;
        float speedH = std::sqrt(vx * vx + vz * vz);

        bool nearSurface = (waterH - particle.position.y) > -0.05f && (waterH - particle.position.y) < 0.20f;
        if (depth > 0.0f || nearSurface || speedH > 0.05f) {
            float du = -vx * p.depositionCoeff * dt * 0.6f;
            float dv = -vz * p.depositionCoeff * dt * 0.6f;

            float tangentialPush = -(vx * dhdx + vz * dhdz);
            float contactLift = -vy * 0.25f;
            float wake = speedH * 0.08f;
            float dh = (tangentialPush + contactLift + wake) * p.depositionCoeff * dt;
            dh = std::max(-0.02f, std::min(0.02f, dh));

            float r = 0.28f;
            water.addRadialImpulse(x, z, r, dh, 0.35f);
            water.addImpulse(x, z, du, dv, 0.0f);
        }
    }
}
