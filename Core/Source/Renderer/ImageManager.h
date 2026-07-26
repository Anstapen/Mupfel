/**
 * \file   TextureManager.h
 * \brief  Load and unload images.
 *
 * \author anton
 * \date   July 2026
 */
#pragma once
#include "Core/Error.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

/** Forward declaration of the underlying image format. */
namespace Ping
{
class Image;
class Device;
class DescriptorSet;
class Pipeline;
} // namespace Ping

namespace Mupfel
{

class ECSRenderer;

typedef uint32_t ImageHandle;

/**
 * This structure is used to describe an animated image.
 * The engine expects animated images in row-major format.
 */
struct ImageSpecification
{
	/** The number of rows in the image. */
	uint32_t rows;
	/** The number of columns in the image. */
	uint32_t columns;
};

/**
 * The main image manager. It supports simple image and more advanced,
 * animated image.
 */
class ImageManager
{
	friend class ECSRenderer;

public:
	/**
	 * Try to load the image given by \a path.
	 *
	 * \param path Path to the image.
	 * \param spec If an animated image should be loaded, this specification is used to interpret the image.
	 * \return ImageHandle or Error Code.
	 */
	[[nodiscard]] Expected<ImageHandle> Load(const Ping::Device& device, const std::string path);

	[[nodiscard]] Expected<ImageHandle> LoadAnimated(
		const Ping::Device&		  device,
		const std::string		  path,
		const ImageSpecification& spec);

	[[nodiscard]] Expected<std::vector<ImageHandle>> LoadSpriteSheet(
		const Ping::Device&		  device,
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
	const std::vector<Ping::Image>& GetImages() const;

private:
	/**
	 * This vector manages the RAII-based images.
	 * ImageHandles directly translate to the vector index.
	 */
	std::vector<Ping::Image> images;
	/**
	 * This map contains a path -> ImageHandle association.
	 * Not used by the engine itself but useful for the user to
	 * be able to reference images by the path.
	 */
	std::unordered_map<std::string, std::vector<ImageHandle>> imageHandleMap;
};

} // namespace Mupfel
