#pragma once

namespace Mupfel
{
/** Placeholder color component; currently just a flat RGBA tint, not an actual texture reference. */
struct Texture
{
	/** Texture index */
	uint32_t index = 0;
	/** Texture repeat factor; values > 1 tile the texture (used by the ground). */
	float uvScale = 1.0f;
};
} // namespace Mupfel