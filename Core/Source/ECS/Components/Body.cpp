#include "ECS/Components/Body.h"
#include "json.hpp"

using namespace  Mupfel;

void Body::BuildFromJSON(const nlohmann::json& source)
{
    auto optional_float_param = [&source](const std::string& name, float& val)
    {
        const auto it = source.find(name);

        if (it != source.end())
        {
            val = it->get<float>();
        }
    };

    auto optional_bool_param = [&source](const std::string& name, bool& val)
    {
        const auto it = source.find(name);

        if (it != source.end())
        {
            val = it->get<bool>();
        }
    };


    std::string s_type = "static";
    const auto it = source.find("type");

    if (it != source.end())
    {
        s_type = it->get<std::string>();
    }

    if (s_type == "kinematic")
    {
        type = BodyType::Kinematic;
    }
    else if (s_type == "dynamic")
    {
        type = BodyType::Dynamic;
    }

    optional_float_param("velocity_x", velocity_x);
    optional_float_param("velocity_y", velocity_y);
    optional_float_param("angular_velocity", angular_velocity);
    optional_float_param("gravity_scale", gravity_scale);
    optional_float_param("linear_damping", linear_damping);
    optional_float_param("angular_damping", angular_damping);

    optional_bool_param("fixed_rotation", fixed_rotation);
    optional_bool_param("is_bullet", is_bullet);
    optional_bool_param("allow_sleep", allow_sleep);
}