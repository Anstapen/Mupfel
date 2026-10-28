#pragma once
#include "ECS/Entity.h"

namespace Mupfel::Testing
{
    struct EntityManagerAccess
    {
        static auto & CurrentEntities(EntityManager &manager) {return manager.current_entities;}
        static auto & NextEntityIndex(EntityManager &manager) {return manager.next_entity_index;}
        static auto & GenerationList(EntityManager &manager) {return manager.generationList;}
        static auto & FreeList(EntityManager &manager) {return manager.freeList;}
        static auto & Alive(EntityManager &manager) {return manager.alive;}
    };
}