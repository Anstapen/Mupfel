#pragma once
#include <functional>
#include <vector>
#include "Api/Entities.h"

namespace Mupfel
{
    class Prefab
    {
    public:
        template <typename T>
        Prefab& With(T component)
        {
            adders.push_back([c = std::move(component)](Entity e) { Entities::AddComponent<T>(e, c); });
            return *this;
        }

        [[nodiscard]] Entity Spawn() const
        {
            const Entity e = Entities::Create();
            for (const auto& add : adders) add(e);
            return e;
        }

    private:
        std::vector<std::function<void(Entity)>> adders;
    };
}