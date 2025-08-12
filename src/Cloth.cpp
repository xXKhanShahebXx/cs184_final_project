#include "Cloth.h"
#include <cmath>
#include "SimpleMath.h"

Cloth::Cloth(int width, int height, float spacing) : windVelocity(Vec3(0.0f)) {
    particles.reserve(width * height);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Vec3 pos(x * spacing, 0.0f, y * spacing);
            particles.emplace_back(pos);
        }
    }
    
    createSprings();
}

void Cloth::calculateNormals() {
    // Reset all normals
    for (auto& p : particles) {
        p.normal = Vec3(0.0f);
    }

    int width = static_cast<int>(sqrt(particles.size()));
    int height = width;

    // Calculate face normals and add them to the vertices
    for (int y = 0; y < height - 1; ++y) {
        for (int x = 0; x < width - 1; ++x) {
            Particle& p1 = particles[y * width + x];
            Particle& p2 = particles[y * width + x + 1];
            Particle& p3 = particles[(y + 1) * width + x + 1];
            Particle& p4 = particles[(y + 1) * width + x];

            // Calculate normals for the two triangles that form the quad: (p1, p2, p3) and (p1, p3, p4)
            //Vec3 normal1 = cross(p2.position - p1.position, p3.position - p1.position);
            Vec3 normal1 = (p2.position - p1.position).cross(p3.position - p1.position);
            p1.normal += normal1;
            p2.normal += normal1;
            p3.normal += normal1;

            //Vec3 normal2 = cross(p3.position - p1.position, p4.position - p1.position);
            Vec3 normal2 = (p3.position - p1.position).cross(p4.position - p1.position);
            p1.normal += normal2;
            p3.normal += normal2;
            p4.normal += normal2;
        }
    }

    // Normalize all the vertex normals for smooth shading
    for (auto& p : particles) {
        p.normal = normalize(p.normal);
    }
}

void Cloth::createSprings() {
    int width = static_cast<int>(sqrt(particles.size()));
    int height = width;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int current = y * width + x;
            
            if (x < width - 1) {
                int right = y * width + (x + 1);
                float restLength = length(particles[right].position - particles[current].position);
                springs.emplace_back(current, right, restLength, 500.0f, 10.0f);
            }
            
            if (y < height - 1) {
                int down = (y + 1) * width + x;
                float restLength = length(particles[down].position - particles[current].position);
                springs.emplace_back(current, down, restLength, 500.0f, 10.0f);
            }
            
            if (x < width - 1 && y < height - 1) {
                int diagonal = (y + 1) * width + (x + 1);
                float restLength = length(particles[diagonal].position - particles[current].position);
                springs.emplace_back(current, diagonal, restLength, 250.0f, 6.0f);
            }
        }
    }
}

void Cloth::update(float deltaTime, const Vec3& gravity, float dragCoefficient, const Vec3& airVelocity) {
    for (auto& particle : particles) particle.force = Vec3(0.0f);

    applySpringForces();

    applyGravity(gravity);
    applyAirDrag(dragCoefficient, airVelocity);

    integrateVelocities(deltaTime);
    integratePositions(deltaTime);
}

void Cloth::prepareForces() {
    for (auto& particle : particles) particle.force = Vec3(0.0f);
    applySpringForces();
}

void Cloth::finalizeIntegration(float deltaTime) {
    integrateVelocities(deltaTime);
    integratePositions(deltaTime);
}

void Cloth::applySpringForces() {
    for (const auto& spring : springs) {
        Particle& p1 = particles[spring.particle1];
        Particle& p2 = particles[spring.particle2];
        
        Vec3 delta = p2.position - p1.position;
        float distance = length(delta);
        
        if (distance > 0.0f) {
            Vec3 direction = delta / distance;
            float displacement = distance - spring.restLength;
            
            Vec3 springForce = direction * displacement * spring.stiffness;
            
            Vec3 relativeVelocity = p2.velocity - p1.velocity;
            Vec3 dampingForce = direction * dot(relativeVelocity, direction) * spring.damping;
            
            Vec3 totalForce = springForce + dampingForce;
            float maxForce = 800.0f;
            float mag = length(totalForce);
            if (mag > maxForce && mag > 0.0f) {
                totalForce = totalForce * (maxForce / mag);
            }
            
            if (!p1.fixed) p1.force += totalForce;
            if (!p2.fixed) p2.force -= totalForce;
        }
    }
}

void Cloth::integrateVelocities(float deltaTime) {
    for (auto& particle : particles) {
        if (!particle.fixed) {
            particle.velocity += (particle.force / particle.mass) * deltaTime;
            particle.velocity = particle.velocity * 0.997f;
        }
    }
}

void Cloth::integratePositions(float deltaTime) {
    for (auto& particle : particles) {
        if (!particle.fixed) {
            particle.position += particle.velocity * deltaTime;
        }
    }
}

void Cloth::applyGravity(const Vec3& gravity) {
    for (auto& particle : particles) {
        if (!particle.fixed) {
            particle.force += gravity * particle.mass;
        }
    }
}

void Cloth::applyAirDrag(float dragCoefficient, const Vec3& airVelocity) {
    for (auto& particle : particles) {
        if (!particle.fixed) {
            Vec3 relativeVelocity = particle.velocity - airVelocity;
            float speed = length(relativeVelocity);
            
            if (speed > 0.0f) {
                Vec3 dragForce = -normalize(relativeVelocity) * speed * dragCoefficient;
                particle.force += dragForce;
            }
        }
    }
}

void Cloth::handleCollision(const Vec3& surfaceNormal, float surfaceHeight) {
    for (auto& particle : particles) {
        if (!particle.fixed) {
            float dotProduct = dot(particle.position, surfaceNormal);
            
            if (dotProduct < surfaceHeight) {
                particle.position = particle.position - surfaceNormal * (dotProduct - surfaceHeight);
                
                float velocityDot = dot(particle.velocity, surfaceNormal);
                if (velocityDot < 0.0f) {
                    particle.velocity = particle.velocity - surfaceNormal * velocityDot * 0.8f;
                }
            }
        }
    }
}

void Cloth::fixCorner(int corner) {
    if (corner >= 0 && corner < static_cast<int>(particles.size())) {
        particles[corner].fixed = true;
    }
}

void Cloth::setInitialVelocity(const Vec3& velocity) {
    for (auto& particle : particles) {
        if (!particle.fixed) {
            particle.velocity = velocity;
        }
    }
} 


