#include "ImageManager.h"
#include "Renderer/Renderer.h"
#include <optional>

using namespace Mupfel;

constexpr uint32_t defaultImageWidth = 1;
constexpr uint32_t defaultImageHeight = 1;

static constexpr unsigned char defaultImage[] = {
	0xff,
	0x00,
	0xff,
	0xff,
};

bool ImageManager::Init(nvrhi::DeviceHandle device)
{ 
	auto desc = nvrhi::TextureDesc()
					.setDimension(nvrhi::TextureDimension::Texture2DArray)
					.setWidth(defaultImageWidth)
					.setHeight(defaultImageHeight)
					.setArraySize(1)
					.setFormat(nvrhi::Format::RGBA8_UNORM)
					.setKeepInitialState(true)
					.setInitialState(nvrhi::ResourceStates::ShaderResource)
					.setDebugName("DefaultImage");

	uploadList = device->createCommandList();
	if (!uploadList)
	{
		Shutdown();
		return false;
	}

	defaultTexture = device->createTexture(desc);

	if (!defaultTexture)
	{
		Shutdown();
		return false;
	}

	uploadList->open();
	uploadList->writeTexture(defaultTexture, 0, 0, defaultImage, defaultImageWidth * 4);
	uploadList->setPermanentTextureState(defaultTexture, nvrhi::ResourceStates::ShaderResource);
	uploadList->commitBarriers();
	uploadList->close();
	device->executeCommandList(uploadList);


	return true;
}

void Mupfel::ImageManager::Shutdown()
{
	defaultTexture = nullptr;
	uploadList = nullptr;
}

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
