/**
 * \file   Images.h
 * \brief  Utilities for quickly loading various images.
 *
 */
#pragma once
#include "Core/Application.h"
#include "Core/Error.h"
#include "Renderer/ImageManager.h"
#include <string>
#include <vector>

namespace Mupfel::Images
{

/**
 * Loads a single, unanimated image.
 *
 * \param path Path to the image, relative to the working directory.
 * \return The image handle, or an error.
 */
[[nodiscard]] inline Expected<ImageHandle> Load(const std::string& path) { return Application::LoadBasicImage(path); }

/**
 * Loads an animated image, cut into spec.rows x spec.columns frames in row-major order.
 *
 *
 * \param path Path to the image.
 * \param spec The frame grid to interpret the image with.
 * \return The image handle, or an error.
 */
[[nodiscard]] inline Expected<ImageHandle> LoadAnimated(const std::string& path, const ImageSpecification& spec)
{
	return Application::LoadAnimatedImage(path, spec);
}

/**
 * Loads a spritesheet as one independent handle per frame.
 *
 *
 * \param path Path to the spritesheet.
 * \param spec The frame grid to cut the sheet with.
 * \return One handle per frame, or an error.
 */
[[nodiscard]] inline Expected<std::vector<ImageHandle>>
LoadSpriteSheet(const std::string& path, const ImageSpecification& spec)
{
	return Application::LoadSpriteSheetImages(path, spec);
}

} // namespace Mupfel::Images
