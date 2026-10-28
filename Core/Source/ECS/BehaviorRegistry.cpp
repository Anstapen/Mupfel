#include "BehaviorRegistry.h"

void Mupfel::BehaviorRegistry::AddEntityBehavior(const std::string& name, const EntityBehavior& behavior)
{
    entityBehaviorMap[name] = behavior;
}

std::optional<Mupfel::EntityBehavior> Mupfel::BehaviorRegistry::FindEntityBehavior(const std::string& name)
{
    const auto it = entityBehaviorMap.find(name);
    if (it != entityBehaviorMap.end())
    {
        return it->second;
    }

    return {};
}