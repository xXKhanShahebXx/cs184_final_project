#pragma once
#include "Cloth.h"
#include "Water.h"

struct CouplingParams {
    float pressureCoeff;
    float dragCoeff;
    float depositionCoeff;
};

void applyWaterToCloth(const WaterGrid& water, Cloth& cloth, const CouplingParams& p);
void applyClothToWater(WaterGrid& water, const Cloth& cloth, const CouplingParams& p, float dt);
