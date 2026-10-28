#pragma once
#include <unordered_map>
#include <functional>
#include "ECS/Entity.h"
#include <string>
#include <optional>

namespace Mupfel
{
    using EntityBehavior = std::function<void (Entity e)>;

    class BehaviorRegistry
    {
    public:
        void AddEntityBehavior(const std::string &name, const EntityBehavior &behavior);
        std::optional<EntityBehavior> FindEntityBehavior(const std::string &name);
    private:
        std::unordered_map<std::string, EntityBehavior> entityBehaviorMap;
    };
}
