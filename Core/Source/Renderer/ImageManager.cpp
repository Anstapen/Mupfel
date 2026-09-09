#include "ImageManager.h"
#include "Renderer/Renderer.h"
#include <optional>

using namespace Mupfel;

Expected<ImageHandle> Mupfel::ImageManager::Load(const std::string path)
{

	if (imageHandleMap.contains(path) && (imageHandleMap[path].size() > 0))
	{
		/* Image is already loaded. */
		return imageHandleMap[path][0];
	}

	/* TODO: Create an image using NVRHI */
	return ImageHandle();
}

Expected<ImageHandle> Mupfel::ImageManager::LoadAnimated(
	const std::string		  path,
	const ImageSpecification& spec)
{
	if (imageHandleMap.contains(path) && (imageHandleMap[path].size() > 0))
	{
		/* Image is already loaded. */
		return imageHandleMap[path][0];
	}

	/* TODO: Create an image using NVRHI */
	(void)spec;
	return ImageHandle();
}

Expected<std::vector<ImageHandle>> Mupfel::ImageManager::LoadSpriteSheet(
	const std::string		  path,
	const ImageSpecification& spec)
{
	if (imageHandleMap.contains(path) && (imageHandleMap[path].size() > 0))
	{
		/* Image is already loaded. */
		return imageHandleMap[path];
	}

	/* Create images */
	(void)spec;

	return {};
}

void Mupfel::ImageManager::Unload(const std::string path)
{
	if (!imageHandleMap.contains(path))
	{
		return;
	}

	for (auto image : imageHandleMap[path])
	{
		Unload(image);
	}
}

void Mupfel::ImageManager::Unload(ImageHandle image) { (void)image; }
