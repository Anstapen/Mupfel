#pragma once
#include <cstdint>
#include "SpatialGrid.h"

class InvaderSystem
{
public:
    void Update(double timestep, const SpatialGrid &grid);
    void SetTPS(uint32_t tps);

private:
    bool UpdateTime(double timestep);

private:
    double currentTime = 0.0f;
    uint32_t ticksPerSecond = 1;
    double tickPeriod = 1.0f;
};
