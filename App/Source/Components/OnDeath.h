#pragma once
#include "ECS/Entity.h"
#include <functional>

struct OnDeath
{
    using Behavior = std::function<void (Mupfel::Entity e)>;
    Behavior behavior = nullptr;
};