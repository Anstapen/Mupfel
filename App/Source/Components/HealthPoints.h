#pragma once
#include <cstdint>
#include <functional>
#include "ECS/Entity.h"
#include "Components/HealthPoints.h"

struct HealthPoints
{
    using Behavior = std::function<void (Mupfel::Entity e, const HealthPoints &hp)>;
    uint32_t value = 1;
    Behavior onHPGain = nullptr;
    Behavior onHPLoss = nullptr;
};