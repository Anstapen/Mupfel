#include "ImageManager.h"
#include "Renderer/Renderer.h"
#include <optional>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace Mupfel;

constexpr uint32_t defaultImageWidth = 1;
constexpr uint32_t defaultImageHeight = 1;

static constexpr unsigned char defaultImage[] = {
	0xff,
	0x00,
	0xff,
	0xff,
};

bool ImageManager::Init(nvrhi::DeviceHandle in_device)
{
	device = in_device;
	nvrhi::TextureDesc desc = nvrhi::TextureDesc()
								  .setDimension(nvrhi::TextureDimension::Texture2DArray)
								  .setWidth(defaultImageWidth)
								  .setHeight(defaultImageHeight)
								  .setArraySize(1)
								  .setFormat(nvrhi::Format::SRGBA8_UNORM)
								  .setKeepInitialState(true)
								  .setInitialState(nvrhi::ResourceStates::ShaderResource)
								  .setDebugName("DefaultImage");

	uploadList = device->createCommandList(nvrhi::CommandListParameters().setEnableImmediateExecution(false));
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

	imageHandleMap["DefaultImage"] = {
		ImageType::ANIMATED, std::vector<ImageHandle>(1, static_cast<ImageHandle>(textureHandles.size()))};
	textureHandles.push_back(defaultTexture);

	return true;
}

void Mupfel::ImageManager::Shutdown()
{
	defaultTexture = nullptr;
	uploadList = nullptr;
	textureHandles.clear();
	freeHandles.clear();
	imageHandleMap.clear();
	device = nullptr;
	imageManagerGeneration++;
}

Expected<ImageHandle> Mupfel::ImageManager::Load(const std::string& path) { return LoadAnimated(path, {1, 1}); }

Expected<ImageHandle> Mupfel::ImageManager::LoadAnimated(const std::string& path, const ImageSpecification& spec)
{
	auto it = imageHandleMap.find(path);
	if (it != imageHandleMap.end() && it->second.handles.size() > 0)
	{
		/* There currently is an image loaded for that path name. */
		if (it->second.type == ImageType::ANIMATED)
		{
			return it->second.handles[0];
		}
		else
		{
			return std::unexpected<Mupfel::Error>(Mupfel::Error::FILE_ALREADY_LOADED);
		}
	}

	auto sheet_or_error = DecodeSheet(path, spec);

	if (!sheet_or_error)
	{
		return std::unexpected<Mupfel::Error>(sheet_or_error.error());
	}

	assert(sheet_or_error.value().subImages.size() > 0);

	nvrhi::TextureHandle handle = CreateTextureAndUpload(sheet_or_error.value());

	if (!handle)
	{
		return std::unexpected<Mupfel::Error>(Mupfel::Error::NO_MEMORY);
	}

	ImageHandle img_handle = AllocateSlot();

	if (img_handle == INVALID_IMAGE)
	{
		return std::unexpected<Mupfel::Error>(Mupfel::Error::NO_MEMORY);
	}
	textureHandles[img_handle] = handle;

	imageHandleMap[path] = {ImageType::ANIMATED, std::vector<ImageHandle>(1, img_handle)};

	imageManagerGeneration++;

	return img_handle;
}

Expected<std::vector<ImageHandle>>
Mupfel::ImageManager::LoadSpriteSheet(const std::string& path, const ImageSpecification& spec)
{
	auto it = imageHandleMap.find(path);
	if (it != imageHandleMap.end() && it->second.handles.size() > 0)
	{
		/* There currently is an image loaded for that path name. */
		if (it->second.type == ImageType::SPRITESHEET)
		{
			return it->second.handles;
		}
		else
		{
			return std::unexpected<Mupfel::Error>(Mupfel::Error::FILE_ALREADY_LOADED);
		}
	}

	auto sheet_or_error = DecodeSheet(path, spec);

	if (!sheet_or_error)
	{
		return std::unexpected<Mupfel::Error>(sheet_or_error.error());
	}

	assert(sheet_or_error.value().subImages.size() > 0);

	std::vector<nvrhi::TextureHandle> handles = CreateTexturesAndUpload(sheet_or_error.value());

	if (handles.size() == 0)
	{
		return std::unexpected<Mupfel::Error>(Mupfel::Error::NO_MEMORY);
	}

	std::vector<ImageHandle> img_handles;

	/* Allocate the images. */
	for (auto& handle : handles)
	{
		ImageHandle img_handle = AllocateSlot();
		if (img_handle == INVALID_IMAGE)
		{
			break;
		}
		textureHandles[img_handle] = handle;
		img_handles.push_back(img_handle);
	}

	/* If the sizes differ, we do not have enough free slots. */
	if (img_handles.size() != handles.size())
	{
		/* Free all allocated handles. */
		for (auto& handle : img_handles)
		{
			Unload(handle);
		}

		return std::unexpected<Mupfel::Error>(Mupfel::Error::NO_MEMORY);
	}

	imageHandleMap[path] = {ImageType::SPRITESHEET, img_handles};

	imageManagerGeneration++;

	return img_handles;
}

void Mupfel::ImageManager::Unload(const std::string& path)
{
	if (!imageHandleMap.contains(path))
	{
		return;
	}

	for (auto image : imageHandleMap[path].handles)
	{
		Unload(image);
	}

	imageHandleMap.erase(path);
}

void Mupfel::ImageManager::Unload(ImageHandle image)
{
	if (image == INVALID_IMAGE || image >= textureHandles.size())
	{
		return;
	}

	/* The deletion of the underlying data is managed by the NVRHI garbage collection. */
	textureHandles[image] = nullptr;

	freeHandles.push_back(image);
	imageManagerGeneration++;
}

nvrhi::TextureHandle Mupfel::ImageManager::GetTextureHandle(ImageHandle handle)
{
	/* We can assume that the default texture lives at textures[0]. */
	assert(textureHandles.size() > 0);
	if (handle >= textureHandles.size() || textureHandles[handle] == nullptr)
	{
		return textureHandles[0];
	}

	return textureHandles[handle];
}

Expected<ImageManager::DecodedSheet>
Mupfel::ImageManager::DecodeSheet(const std::string& path, const ImageSpecification& spec)
{
	if (spec.columns == 0 || spec.rows == 0)
	{
		return std::unexpected<Mupfel::Error>(Mupfel::Error::INVALID_PARAMETER);
	}

	int		 texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		return std::unexpected<Mupfel::Error>(Mupfel::Error::FILE_NOT_FOUND);
	}

	if ((texWidth % spec.columns != 0) || (texHeight % spec.rows != 0))
	{
		stbi_image_free(pixels);
		return std::unexpected<Mupfel::Error>(Mupfel::Error::FILE_WRONG_FORMAT);
	}

	std::vector<std::vector<uint8_t>> subImages;
	uint8_t*						  pixel_ptr = pixels;
	uint32_t						  subImageWidth = texWidth / spec.columns;
	uint32_t						  subImageHeight = texHeight / spec.rows;
	uint32_t						  subImageSize = subImageWidth * subImageHeight * 4;

	/* copy the image data into ub buffers. */
	for (uint32_t row = 0; row < spec.rows; row++)
	{
		for (uint32_t column = 0; column < spec.columns; column++)
		{
			uint64_t			 starting_offset = (static_cast<uint64_t>(row) * subImageHeight * texWidth +
													static_cast<uint64_t>(column) * subImageWidth) *
												   4;
			std::vector<uint8_t> subImage;
			subImage.reserve(subImageSize);

			for (uint32_t i = 0; i < subImageHeight; i++)
			{
				for (uint32_t k = 0; k < subImageWidth; k++)
				{
					uint64_t offset = starting_offset + (static_cast<uint64_t>(i) * texWidth + k) * 4;
					for (uint32_t l = 0; l < 4; l++)
					{

						subImage.push_back(pixel_ptr[offset + l]);
					}
				}
			}

			subImages.push_back(std::move(subImage));
		}
	}

	stbi_image_free(pixels);

	return ImageManager::DecodedSheet{subImageWidth, subImageHeight, std::move(subImages)};
}

nvrhi::TextureHandle Mupfel::ImageManager::CreateTextureAndUpload(DecodedSheet& sheet)
{
	nvrhi::TextureDesc desc = nvrhi::TextureDesc()
								  .setDimension(nvrhi::TextureDimension::Texture2DArray)
								  .setWidth(sheet.subWidth)
								  .setHeight(sheet.subHeight)
								  .setArraySize(static_cast<uint32_t>(sheet.subImages.size()))
								  .setFormat(nvrhi::Format::SRGBA8_UNORM)
								  .setKeepInitialState(true)
								  .setInitialState(nvrhi::ResourceStates::ShaderResource);

	nvrhi::TextureHandle handle = device->createTexture(desc);

	if (!handle)
	{
		return handle;
	}

	/* Upload to GPU. */
	uploadList->open();

	for (uint32_t i = 0; i < sheet.subImages.size(); i++)
	{
		uploadList->writeTexture(handle, i, 0, sheet.subImages[i].data(), static_cast<size_t>(sheet.subWidth) * 4);
	}

	uploadList->setPermanentTextureState(handle, nvrhi::ResourceStates::ShaderResource);
	uploadList->commitBarriers();
	uploadList->close();
	device->executeCommandList(uploadList);

	return handle;
}

std::vector<nvrhi::TextureHandle> Mupfel::ImageManager::CreateTexturesAndUpload(DecodedSheet& sheet)
{
	nvrhi::TextureDesc desc = nvrhi::TextureDesc()
								  .setDimension(nvrhi::TextureDimension::Texture2DArray)
								  .setWidth(sheet.subWidth)
								  .setHeight(sheet.subHeight)
								  .setArraySize(1)
								  .setFormat(nvrhi::Format::SRGBA8_UNORM)
								  .setKeepInitialState(true)
								  .setInitialState(nvrhi::ResourceStates::ShaderResource);

	std::vector<nvrhi::TextureHandle> handles;

	/* First, allocate all the texture handles. */
	for (uint32_t i = 0; i < sheet.subImages.size(); i++)
	{
		nvrhi::TextureHandle handle = device->createTexture(desc);

		if (!handle)
		{
			handles.clear();
			return handles;
		}

		handles.push_back(handle);
	}

	assert(handles.size() == sheet.subImages.size());

	uploadList->open();

	for (uint32_t i = 0; i < handles.size(); i++)
	{
		uploadList->writeTexture(handles[i], 0, 0, sheet.subImages[i].data(), static_cast<size_t>(sheet.subWidth) * 4);
		uploadList->setPermanentTextureState(handles[i], nvrhi::ResourceStates::ShaderResource);
	}

	uploadList->commitBarriers();
	uploadList->close();
	device->executeCommandList(uploadList);

	return handles;
}

ImageHandle Mupfel::ImageManager::AllocateSlot()
{
	if (!freeHandles.empty())
	{
		ImageHandle slot = freeHandles.back();
		freeHandles.pop_back();
		return slot;
	}

	if (textureHandles.size() >= Mupfel::MAX_IMAGE_COUNT)
	{
		return INVALID_IMAGE;
	}

	textureHandles.push_back(nullptr);
	return static_cast<ImageHandle>(textureHandles.size() - 1);
}
