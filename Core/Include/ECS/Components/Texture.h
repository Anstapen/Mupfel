#pragma once
#include <cstdint>
#include "json_fwd.hpp"

namespace Mupfel
{
    struct Texture
    {
        /** Texture index */
        uint32_t index = 0;
        float scale_x = 1.0f;
        float scale_y = 1.0f;

        bool BuildFromJSON(const nlohmann::json& source);
    };
} // namespace Mupfel
