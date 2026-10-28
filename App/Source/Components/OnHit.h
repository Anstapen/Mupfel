#pragma once
#include "ECS/Entity.h"
#include <functional>

struct OnHit
{
    using Behavior = std::function<void (Mupfel::Entity e)>;
    Behavior behavior = nullptr;
};