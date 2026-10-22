#pragma once
#include "ECS/ComponentArray.h"

namespace Mupfel::Testing
{
    struct ComponentArrayAccess
    {
        template <ComponentType T>
        static auto& Sparse(const ComponentArray<T>& a) { return a.sparse; }

        template <ComponentType T>
        static auto& Dense(const ComponentArray<T>& a) { return a.dense; }

        template <ComponentType T>
        static auto& Components(const ComponentArray<T>& a) { return a.components; }

        template <ComponentType T>
        static auto& PendingOperations(const ComponentArray<T>& a) { return a.pendingOperations; }
    };
}
