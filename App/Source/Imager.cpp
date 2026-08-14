#include "Imager.h"
#include <expected>

using namespace Mupfel;

std::unordered_map<std::string, ImageHandle> Imager::image_map;
std::unordered_map<std::string, std::vector<Mupfel::ImageHandle>> Imager::spritesheet_map;

bool Imager::Load(const std::string& in_handle, const std::string& path)
{
	auto img_handle = Images::Load(path).transform([&in_handle](ImageHandle handle) { image_map[in_handle] = handle; });

	return img_handle.has_value();
}

bool Imager::LoadAnimated(const std::string& in_handle, const std::string& path, const ImageSpecification& spec)
{
	auto img_handle =
		Images::LoadAnimated(path, spec).transform([&in_handle](ImageHandle handle) { image_map[in_handle] = handle; });

	return img_handle.has_value();
}

bool Imager::LoadSpriteSheet(const std::string& in_handle, const std::string& path, const ImageSpecification& spec)
{
	auto img_handle = Images::LoadSpriteSheet(path, spec)
						  .transform([&in_handle](std::vector<ImageHandle> handles) { spritesheet_map[in_handle] = handles; });
	return img_handle.has_value();
}

ImageHandle Imager::Get(const std::string& in_handle) { return image_map[in_handle]; }

const std::vector<Mupfel::ImageHandle>& Imager::GetSpriteSheet(const std::string& in_handle)
{
	return spritesheet_map[in_handle];
}
