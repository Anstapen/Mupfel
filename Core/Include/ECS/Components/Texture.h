#pragma once
#include <cstdint>

namespace Mupfel
{
struct Texture
{
	/** Texture index */
	uint32_t index = 0;
	float scale_x = 1.0f;
	float scale_y = 1.0f;
};
} // namespace Mupfel