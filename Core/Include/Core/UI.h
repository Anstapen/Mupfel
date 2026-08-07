#pragma once
#include <string>
#include <cstdint>

namespace Mupfel
{
class UI
{
public:
	/**
	 * Draw a button using the given image.
	 * 
	 * 
	 * \param x x-offset in screen space.
	 * \param y y-offset in screen space.
	 * \param width The width of the button. This is independent of the used texture.
	 * \param height The height of the button. This is independent of the used texture.
	 * \param image_path Path to an image containing the button texture.
	 * \return 0 if the cursor is not overlapping the button, 1 if the cursor is hovering over the button, 2 if the
	 * button is pressed. 
	 */
	static uint32_t Button(float x, float y, float width, float height, const std::string& image_path);
};
} // namespace Mupfel
