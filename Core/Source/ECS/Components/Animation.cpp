#include "ECS/Components/Animation.h"
#include "json.hpp"

using namespace Mupfel;

void Animation::BuildFromJSON(const nlohmann::json& source)
{
    auto it = source.find("first_frame");

    if (it != source.end())
    {
        firstFrame = it->get<uint32_t>();
    }

    it = source.find("frame_count");

    if (it != source.end())
    {
        frameCount = it->get<uint32_t>();
    }

    it = source.find("fps");

    if (it != source.end())
    {
        fps = it->get<float>();
    }

    it = source.find("elapsed_time");

    if (it != source.end())
    {
        elapsed = it->get<float>();
    }

    it = source.find("repeating");

    if (it != source.end())
    {
        repeating = it->get<bool>();
    }

    it = source.find("auto_start");

    if (it != source.end())
    {
        finished = !it->get<bool>();
    }
}