#include "ECS/Components/Sensor.h"
#include "json.hpp"

using namespace Mupfel;

void Sensor::BuildFromJSON(const nlohmann::json& source)
    {
        auto optional_float_param = [&source](const std::string& name, float& val)
        {
            const auto it = source.find(name);

            if (it != source.end())
            {
                val = it->get<float>();
            }
        };

        auto optional_uint64_param = [&source](const std::string& name, uint64_t& val)
        {
            const auto it = source.find(name);

            if (it != source.end())
            {
                val = it->get<uint64_t>();
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

        std::string sensor_type = "box";

        const auto it = source.find("type");
        if (it != source.end())
        {
            sensor_type = it->get<std::string>();
        }


        if (sensor_type == "box")
        {
            float width = 1.0f;
            optional_float_param("width", width);

            float height = 1.0f;
            optional_float_param("height", height);

            SetBox(width, height);
        }
        else if (sensor_type == "circle")
        {
            float radius = 1.0f;
            optional_float_param("radius", radius);
            SetCircle(radius);
        }
        else if (sensor_type == "capsule")
        {
            float radius = 1.0f;
            optional_float_param("radius", radius);

            float center1_x = 0.5f;
            optional_float_param("center1_x", center1_x);

            float center1_y = 0.0f;
            optional_float_param("center1_y", center1_y);

            float center2_x = -0.5f;
            optional_float_param("center2_x", center2_x);

            float center2_y = 0.0f;
            optional_float_param("center2_y", center2_y);

            SetCapsule(center1_x, center1_y, center2_x, center2_y, radius);
        }

        optional_float_param("offset_x", offset_x);
        optional_float_param("offset_y", offset_y);
        optional_uint64_param("category", category);
        optional_uint64_param("mask", mask);
        optional_bool_param("report_events", report_events);
    }