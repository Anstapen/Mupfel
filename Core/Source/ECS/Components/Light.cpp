#include "ECS/Components/Light.h"
#include "json.hpp"

using namespace Mupfel;

void Light::BuildFromJSON(const nlohmann::json& source)
{
    auto optional_float_param = [&source](const std::string& name, float& val)
    {
        const auto it = source.find(name);

        if (it != source.end())
        {
            val = it->get<float>();
        }
    };

    optional_float_param("ambient_strength", ambientStrength);
    optional_float_param("r", r);
    optional_float_param("g", g);
    optional_float_param("b", b);
}
