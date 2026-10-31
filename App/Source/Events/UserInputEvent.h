#pragma once
#include "Core/Event.h"

struct MovementEvent : Mupfel::Event
{
    enum class Direction : uint8_t
    {
        NONE,
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    explicit MovementEvent(Direction in_dir, bool in_started) : dir(in_dir), started(in_started)
    {
    }

    Direction dir;
    bool started;
};

struct UserAction : Mupfel::Event
{
    enum class Action : uint8_t
    {
        NONE,
        GENERATE_ENTITY
    };

    UserAction(Action in_action) : action(in_action) {}

    Action action;
};