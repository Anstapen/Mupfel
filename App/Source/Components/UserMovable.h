#pragma once
#include <functional>
#include "Events/UserInputEvent.h"
#include "ECS/Entity.h"

struct UserMovable
{
    using Update = std::function<void (Mupfel::Entity ent, const MovementEvent &evt)>;

    Update onUpdate = nullptr;
};