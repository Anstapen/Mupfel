#pragma once
#include "ECS/Entity.h"

namespace Mupfel::Testing
{
    struct EntityAccess
    {
        static auto& Index(Entity &e) { return e.index; }
        static auto& Gen(Entity &e) { return e.generation; }
        static Entity Create(uint32_t index, uint32_t gen) { return Entity(index, gen); }
    };
}
