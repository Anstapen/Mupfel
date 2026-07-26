#include "ImageManager.h"
#include "Ping/Device.h"
#include "Ping/Image.h"
#include "Renderer/Renderer.h"
#include <optional>

using namespace Mupfel;

Expected<ImageHandle> Mupfel::ImageManager::Load(const Ping::Device& device, const std::string path)
{

	if (imageHandleMap.contains(path) && (imageHandleMap[path].size() > 0))
	{
		/* Image is already loaded. */
		return imageHandleMap[path][0];
	}

	std::optional<Ping::Image> image = device.CreateImage(path, Ping::ImageUsage::Sampled);

	if (!image.has_value())
	{
		return std::unexpected<Error>(Error::FILE_NOT_FOUND);
	}

	ImageHandle handle = static_cast<uint32_t>(images.size());

	images.emplace_back(std::move(image.value()));

	imageHandleMap[path].push_back(handle);


	return handle;
}

Expected<ImageHandle> Mupfel::ImageManager::LoadAnimated(
	const Ping::Device&		  device,
	const std::string		  path,
	const ImageSpecification& spec)
{
	if (imageHandleMap.contains(path) && (imageHandleMap[path].size() > 0))
	{
		/* Image is already loaded. */
		return imageHandleMap[path][0];
	}

	std::optional<Ping::Image> image = device.CreateImage(path, Ping::ImageUsage::Sampled, spec.rows, spec.columns);

	if (!image.has_value())
	{
		return std::unexpected<Error>(Error::FILE_NOT_FOUND);
	}

	ImageHandle handle = static_cast<uint32_t>(images.size());

	images.emplace_back(std::move(image.value()));

	imageHandleMap[path].push_back(handle);

	return handle;
}

Expected<std::vector<ImageHandle>> Mupfel::ImageManager::LoadSpriteSheet(
	const Ping::Device&		  device,
	const std::string		  path,
	const ImageSpecification& spec)
{
	if (imageHandleMap.contains(path) && (imageHandleMap[path].size() > 0))
	{
		/* Image is already loaded. */
		return imageHandleMap[path];
	}

	std::optional<std::vector<Ping::Image>> created_images =
		device.CreateImages(path, Ping::ImageUsage::Sampled, spec.rows, spec.columns);

	if (!created_images.has_value())
	{
		return std::unexpected<Error>(Error::FILE_NOT_FOUND);
	}

	if (created_images.value().size() == 0)
	{
		return std::unexpected<Error>(Error::FILE_NOT_FOUND);
	}

	std::vector<ImageHandle> handles;
	ImageHandle				 handle = static_cast<uint32_t>(images.size());

	for (uint32_t i = 0; i < created_images.value().size(); i++)
	{
		handles.push_back(handle);
		images.emplace_back(std::move(created_images.value()[i]));
		handle = static_cast<uint32_t>(images.size());
	}

	imageHandleMap[path] = handles;

	return handles;
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

void Mupfel::ImageManager::Unload(ImageHandle image) {}

const std::vector<Ping::Image>& Mupfel::ImageManager::GetImages() const
{ return images; }
