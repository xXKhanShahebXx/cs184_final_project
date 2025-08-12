#pragma once
#include <vector>
#include "SimpleMath.h"

struct Particle {
    Vec3 position;
    Vec3 velocity;
    Vec3 force;
    Vec3 normal;
    float mass;
    bool fixed;
    
    Particle(const Vec3& pos, float m = 1.0f) 
        : position(pos), velocity(Vec3(0.0f)), force(Vec3(0.0f)), mass(m), fixed(false) {}
};

struct Spring {
    int particle1;
    int particle2;
    float restLength;
    float stiffness;
    float damping;
    
    Spring(int p1, int p2, float rest, float k = 100.0f, float d = 5.0f)
        : particle1(p1), particle2(p2), restLength(rest), stiffness(k), damping(d) {}
};

class Cloth {
public:
    Cloth(int width, int height, float spacing = 0.1f);
    ~Cloth() = default;
    
    void update(float deltaTime, const Vec3& gravity, float dragCoefficient, const Vec3& airVelocity);
    void applyGravity(const Vec3& gravity);
    void applyAirDrag(float dragCoefficient, const Vec3& airVelocity);
    void handleCollision(const Vec3& surfaceNormal, float surfaceHeight);

    void calculateNormals();
    
    void prepareForces();
    void finalizeIntegration(float deltaTime);
    
    const std::vector<Particle>& getParticles() const { return particles; }
    std::vector<Particle>& getParticles() { return particles; }
    const std::vector<Spring>& getSprings() const { return springs; }
    
    void fixCorner(int corner);
    void setWind(const Vec3& windVel) { windVelocity = windVel; }
    void setInitialVelocity(const Vec3& velocity);

private:
    std::vector<Particle> particles;
    std::vector<Spring> springs;
    Vec3 windVelocity;
    
    void createSprings();
    void applySpringForces();
    void integrateVelocities(float deltaTime);
    void integratePositions(float deltaTime);
}; 