#pragma once
#include "Core/Logger.h"
#include "Mupfel.h"
#include <cstdint>
#include <string>
#include <unordered_map>

class Player
{
public:
    Player() = default;
    void Init();

private:
    Mupfel::Logger::SafeLoggerPtr logger;
    float velocity_x = 0.0f;
    float velocity_y = 0.0f;
    float movement_speed = 3.0f;
};
