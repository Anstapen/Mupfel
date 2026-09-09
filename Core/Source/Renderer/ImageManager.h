/**
 * \file   TextureManager.h
 * \brief  Load and unload images.
 *
 * \author anton
 * \date   July 2026
 */
#pragma once
#include "Core/Error.h"
#include "Renderer/Image.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "nvrhi/nvrhi.h"

namespace Mupfel
{

class ECSRenderer;
class IMRenderer;

/**
 * The main image manager. It supports simple image and more advanced,
 * animated image.
 */
class ImageManager
{
	friend class ECSRenderer;
	friend class IMRenderer;

public:
	bool Init(nvrhi::DeviceHandle device);
	void Shutdown();
	/**
	 * Try to load the image given by \a path.
	 *
	 * \param path Path to the image.
	 * \param spec If an animated image should be loaded, this specification is used to interpret the image.
	 * \return ImageHandle or Error Code.
	 */
	[[nodiscard]] Expected<ImageHandle> Load(const std::string path);

	[[nodiscard]] Expected<ImageHandle> LoadAnimated(const std::string path, const ImageSpecification& spec);

	[[nodiscard]] Expected<std::vector<ImageHandle>>
	LoadSpriteSheet(const std::string path, const ImageSpecification& spec);

	void Unload(const std::string path);

	void Unload(ImageHandle image);

private:
	/**
	 * This map contains a path -> ImageHandle association.
	 * Not used by the engine itself but useful for the user to
	 * be able to reference images by the path.
	 */
	std::unordered_map<std::string, std::vector<ImageHandle>> imageHandleMap;

	/** A default texture that is displayed when a given texture id does not refer to an existing texture. */
	nvrhi::TextureHandle defaultTexture;

	/** Command list to upload CPU images to the GPU. */
	nvrhi::CommandListHandle uploadList;
};

} // namespace Mupfel
