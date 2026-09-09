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
	/**
	 * Try to load the image given by \a path.
	 *
	 * \param path Path to the image.
	 * \param spec If an animated image should be loaded, this specification is used to interpret the image.
	 * \return ImageHandle or Error Code.
	 */
	[[nodiscard]] Expected<ImageHandle> Load(const std::string path);

	[[nodiscard]] Expected<ImageHandle> LoadAnimated(
		const std::string		  path,
		const ImageSpecification& spec);

	[[nodiscard]] Expected<std::vector<ImageHandle>> LoadSpriteSheet(
		const std::string		  path,
		const ImageSpecification& spec);

	void Unload(const std::string path);

	void Unload(ImageHandle image);

public:
	/**
	 * The maximum number of images that can be opened concurrently.
	 */
	static constexpr uint32_t maxImageCount = 4096;

private:
	/**
	 * This map contains a path -> ImageHandle association.
	 * Not used by the engine itself but useful for the user to
	 * be able to reference images by the path.
	 */
	std::unordered_map<std::string, std::vector<ImageHandle>> imageHandleMap;
};

} // namespace Mupfel
