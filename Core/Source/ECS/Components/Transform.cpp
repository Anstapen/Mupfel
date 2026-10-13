#include "ECS/Components/Transform.h"
#include "json.hpp"

using namespace Mupfel;

void Transform::BuildFromJSON(const nlohmann::json& source)
{
    auto optional_float_param = [&source](const std::string& name, float& val)
    {
        const auto it = source.find(name);

        if (it != source.end())
        {
            val = it->get<float>();
        }
    };

    optional_float_param("pos_x", pos_x);
    optional_float_param("pos_y", pos_y);
    optional_float_param("pos_z", pos_z);
    optional_float_param("rotation", rotation);
}
