#pragma once
#include "Core/Event.h"
#include "ECS/Entity.h"

struct InvaderDestroyedEvent : Mupfel::Event
{
    InvaderDestroyedEvent(Mupfel::Entity in_e) : e(in_e) {}

    Mupfel::Entity e;
};