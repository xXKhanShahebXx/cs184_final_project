#pragma once
#include <vector>
#include "SimpleMath.h"

class WaterGrid {
public:
    WaterGrid(int nx, int nz, float dx, const Vec3& origin, float baseLevel);

    void step(float dt);

    float sampleHeight(float x, float z) const;
    Vec3 sampleVelocity(float x, float z) const;

    void addImpulse(float x, float z, float du, float dv, float dh);

    int getNx() const { return nx; }
    int getNz() const { return nz; }
    float getDx() const { return dx; }
    const Vec3& getOrigin() const { return origin; }
    float getBaseLevel() const { return baseLevel; }

    const std::vector<float>& getH() const { return h; }

private:
    int nx, nz;
    float dx;
    Vec3 origin;
    float baseLevel;

    std::vector<float> h;
    std::vector<float> u;
    std::vector<float> v;

    std::vector<float> hTmp;
    std::vector<float> uTmp;
    std::vector<float> vTmp;

    float gravity;
    float viscosity;

    int idx(int i, int k) const { return k * nx + i; }
    void applyBoundary();
    void diffuse(float dt);
    void advect(float dt);
    void project(float dt);
    void addHeightDamping(float dt);
};
