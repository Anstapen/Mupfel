#include "ECS/Components/Texture.h"
#include "json.hpp"
#include "Core/Application.h"

using namespace Mupfel;

bool Texture::BuildFromJSON(const nlohmann::json& source)
{
    auto it = source.find("path");

    if (it == source.end())
    {
        return false;
    }

    const std::string path = it->get<std::string>();

    /* The image type only needs to specified if animated images or spritesheets are wanted. */
    std::string image_type("basic");
    it = source.find("type");

    if (it != source.end())
    {
        image_type = it->get<std::string>();
    }

    it = source.find("scale_x");

    if (it != source.end())
    {
        scale_x = it->get<float>();
    }

    it = source.find("scale_y");

    if (it != source.end())
    {
        scale_y = it->get<float>();
    }

    /* If the image type is "basic, we have all the information that is needed. */
    if (image_type == "basic")
    {
        const ImageHandle handle = Application::LoadBasicImage(path).value_or(INVALID_IMAGE);
        index = handle;
        return true;
    }

    it = source.find("columns");

    if (it == source.end())
    {
        return false;
    }

    const uint32_t columns = it->get<uint32_t>();

    it = source.find("rows");

    if (it == source.end())
    {
        return false;
    }

    const uint32_t rows = it->get<uint32_t>();

    if (image_type == "spritesheet")
    {
        const auto handles = Application::LoadSpriteSheetImages(path, {rows, columns});
        if (handles && !handles->empty())
        {
            index = handles.value()[0];
        }
    }
    else if (image_type == "animated")
    {
        index = Application::LoadAnimatedImage(path, {rows, columns}).value_or(INVALID_IMAGE);
    }
    else
    {
        return false;
    }

    return true;
}
