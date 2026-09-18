/**
 * \file   Images.h
 * \brief  Utilities for quickly loading various images.
 *
 */
#pragma once
#include "Core/Application.h"
#include "Core/Error.h"
#include "Renderer/Image.h"
#include <string>
#include <vector>

namespace Mupfel::Images
{

/**
 * Loads a single, unanimated image.
 *
 * \param path Path to the image, relative to the working directory.
 * \param mngr An optional resource manager to load the data from.
 * \return The image handle, or an error.
 */
[[nodiscard]] inline Expected<ImageHandle> Load(const std::string& path, Mupfel::ResourceManager* mngr = nullptr)
{
	return Application::LoadBasicImage(path, mngr);
}

/**
 * Loads an animated image, cut into spec.rows x spec.columns frames in row-major order.
 *
 *
 * \param path Path to the image.
 * \param spec The frame grid to interpret the image with.
 * \param mngr An optional resource manager to load the data from.
 * \return The image handle, or an error.
 */
[[nodiscard]] inline Expected<ImageHandle>
LoadAnimated(const std::string& path, const ImageSpecification& spec, Mupfel::ResourceManager* mngr = nullptr)
{
	return Application::LoadAnimatedImage(path, spec, mngr);
}

/**
 * Loads a spritesheet as one independent handle per frame.
 *
 *
 * \param path Path to the spritesheet.
 * \param spec The frame grid to cut the sheet with.
 * \param mngr An optional resource manager to load the data from.
 * \return One handle per frame, or an error.
 */
[[nodiscard]] inline Expected<std::vector<ImageHandle>>
LoadSpriteSheet(const std::string& path, const ImageSpecification& spec, Mupfel::ResourceManager* mngr = nullptr)
{
	return Application::LoadSpriteSheetImages(path, spec, mngr);
}

} // namespace Mupfel::Images
