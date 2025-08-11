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
            Vec3 dragForce = -relVel * p.dragCoeff;
            particle.force += pressureForce + dragForce;
        }
    }
}

void applyClothToWater(WaterGrid& water, const Cloth& cloth, const CouplingParams& p, float dt) {
    const auto& particles = cloth.getParticles();
    for (const auto& particle : particles) {
        float waterH = water.sampleHeight(particle.position.x, particle.position.z);
        float depth = std::max(0.0f, waterH - particle.position.y);
        if (depth > 0.0f || particle.velocity.y < -0.1f) {
            float du = -particle.velocity.x * p.depositionCoeff * dt;
            float dv = -particle.velocity.z * p.depositionCoeff * dt;
            float dh = -particle.velocity.y * 0.2f * p.depositionCoeff * dt;
            water.addImpulse(particle.position.x, particle.position.z, du, dv, dh);
        }
    }
}
