#pragma once
#include "json_fwd.hpp"

namespace Mupfel
{
    struct Light
    {
        float ambientStrength = 0.1f;
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;

        void BuildFromJSON(const nlohmann::json& source);
    };
} // namespace Mupfel
