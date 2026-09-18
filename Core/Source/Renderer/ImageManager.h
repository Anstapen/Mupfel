/**
 * \file   ImageManager.h
 * \brief  This header declares the ImageManager
 * class, used to upload images from cpu memory
 * (indicated by the path relative to the working
 * directory) to the GPU, to be used by the
 * renderer.
 *
 * \author Anton Stapenhorst
 * \date   September 2026
 */
#pragma once
#include "Core/Error.h"
#include "Renderer/Image.h"
#include "ResourceManager.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "nvrhi/nvrhi.h"

namespace Mupfel
{

/** The Image Manager can load images from the CPU into GPU-bound images. */
class ImageManager
{

public:
	/**
	 * Initialize the Image Manager.
	 *
	 * This initializes all needed NVRHI data and uploads a default texture.
	 *
	 * \param in_device The NVRHI device to use for initialization.
	 * \return True if the initialization was successful, false otherwise.
	 */
	bool Init(nvrhi::DeviceHandle in_device);

	/**
	 * Shutdown the Image Manager object.
	 *
	 * This can be used to explicitly shut down the Image Manager, if object desctruction
	 * is not an option.
	 *
	 */
	void Shutdown();

	/**
	 * Try to load the image given by \a path.
	 *
	 * \param path The filesystem path to the image.
	 * \return ImageHandle or error code.
	 * \retval Error::FILE_ALREADY_LOADED A spritesheet with the same name is already loaded.
	 * \retval Error::NO_MEMORY Not enough memory to upload the image.
	 * \retval Error::FILE_NOT_FOUND The image could not be found.
	 */
	[[nodiscard]] Expected<ImageHandle> Load(const std::string& path, ResourceManager* mngr = nullptr);

	/**
	 * Load an animated image into GPU memory.
	 *
	 * The raw image data is interpreted as columns * rows sub-images.
	 *
	 * \param path The filesystem path to the image.
	 * \param spec How the image should be interpreted.
	 * \return ImageHandle or error code.
	 * \retval Error::FILE_ALREADY_LOADED A spritesheet with the same name is already loaded.
	 * \retval Error::NO_MEMORY Not enough memory to upload the image.
	 * \retval Error::FILE_NOT_FOUND The image could not be found.
	 * \retval Error::INVALID_PARAMETER If spec.colums or spec.rows is zero.
	 * \retval Error::FILE_WRONG_FORMAT If the image dimensions are incompatible with \a spec.
	 */
	[[nodiscard]] Expected<ImageHandle>
	LoadAnimated(const std::string& path, const ImageSpecification& spec, ResourceManager* mngr = nullptr);

	/**
	 * Load a spritesheet into GPU memory.
	 *
	 * The raw image data is interpreted using the given spec. All subimages
	 * are distinct images with a handle.
	 *
	 * \param path The filesystem path to the image.
	 * \param spec How the image should be interpreted.
	 * \return The ImageHandles for all the spritesheet images or error code.
	 * \retval Error::FILE_ALREADY_LOADED An animated image with the same name is already loaded.
	 * \retval Error::NO_MEMORY Not enough memory to upload the image.
	 * \retval Error::FILE_NOT_FOUND The image could not be found.
	 * \retval Error::INVALID_PARAMETER If spec.colums or spec.rows is zero.
	 * \retval Error::FILE_WRONG_FORMAT If the image dimensions are incompatible with \a spec.
	 */
	[[nodiscard]] Expected<std::vector<ImageHandle>>
	LoadSpriteSheet(const std::string& path, const ImageSpecification& spec, ResourceManager* mngr = nullptr);

	/**
	 * Unload an image using the filesystem path.
	 *
	 * \param path The filesystem path to the image.
	 */
	void Unload(const std::string& path);

	/**
	 * Retrieve a texture handle from an image handle.
	 *
	 * If no texture was found for the given image handle, the default texture will be returned.
	 *
	 * \param handle The image handle.
	 * \return The texture handle.
	 */
	nvrhi::TextureHandle GetTextureHandle(ImageHandle handle);

	/**
	 * Return the texture handles.
	 * Used by sub-renderers to update their descriptor sets.
	 *
	 * \return All texture handles.
	 */
	[[nodiscard]] const std::vector<nvrhi::TextureHandle>& GetTextureHandles() const { return textureHandles; }

	/**
	 * Return the current generation of this ImageManager object.
	 *
	 * \return The current generation.
	 */
	[[nodiscard]] uint32_t GetGeneration() const { return imageManagerGeneration; }

private:
	/**
	 * Information about a decoded sprite sheet or animated image.
	 */
	struct DecodedSheet
	{
		uint32_t						  subWidth = 0;
		uint32_t						  subHeight = 0;
		std::vector<std::vector<uint8_t>> subImages;
	};

	enum class ImageType
	{
		NONE,
		ANIMATED,
		SPRITESHEET
	};

	/**
	 * Information about an image which was already loaded.
	 */
	struct ImageInfo
	{
		ImageType				 type;
		std::vector<ImageHandle> handles;
	};

	/**
	 * Decodes raw image data based on the given specification.
	 *
	 * \param path The filesystem path to the image.
	 * \param spec How the image should be interpreted.
	 * \return The decoded sheet with subimages or an error.
	 */
	static Expected<DecodedSheet>
	DecodeSheet(const std::string& path, const ImageSpecification& spec, ResourceManager* mngr);

	/**
	 * Create a layered texture from image data.
	 *
	 * \param device The device that is used to create the handles.
	 * \param sheet The decoded image.
	 * \return The texture handle.
	 */
	nvrhi::TextureHandle CreateTextureAndUpload(DecodedSheet& sheet);

	/**
	 * Create texture handles from spritesheet.
	 *
	 * \param device The device that is used to create the handles.
	 * \param sheet The decoded sprite sheet.
	 * \return An array of texture handles.
	 */
	std::vector<nvrhi::TextureHandle> CreateTexturesAndUpload(DecodedSheet& sheet);

	/**
	 * Allocate a new ImageHandle.
	 *
	 * This either pushes a new texture handle into the vector or takes one from the free list.
	 *
	 * \return The image handle.
	 */
	ImageHandle AllocateSlot();

	/**
	 * Unload an image using the ImageHandle.
	 *
	 * \param image The handle to the image.
	 */
	void Unload(ImageHandle image);

private:
	/**
	 * This map contains a path -> ImageHandle association.
	 * Not used by the engine itself but useful for the user to
	 * be able to reference images by the path.
	 */
	std::unordered_map<std::string, ImageInfo> imageHandleMap;

	/** This vector contains the ImageHandle -> nvrhi::TextureHandle association. */
	std::vector<nvrhi::TextureHandle> textureHandles;

	/** A free list that holds ImageHandles that can be reused. */
	std::vector<ImageHandle> freeHandles;

	/** A default texture that is displayed when a given texture id does not refer to an existing texture. */
	nvrhi::TextureHandle defaultTexture;

	/** Command list to upload CPU images to the GPU. */
	nvrhi::CommandListHandle uploadList;

	/** The NVRHI device. */
	nvrhi::DeviceHandle device;

	/**
	 * This marks the current generation of the ImageManager. This value
	 * is monotonically increased each time an image is (un-)loaded.
	 * It is used by the sub-renderers to update their descriptor sets.
	 */
	uint32_t imageManagerGeneration = 0;
};

} // namespace Mupfel
