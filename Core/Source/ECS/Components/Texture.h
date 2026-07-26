#pragma once

namespace Mupfel
{
/** Placeholder color component; currently just a flat RGBA tint, not an actual texture reference. */
struct Texture
{
	/** Texture index */
	uint32_t index;
	/** If true, rendered as an upright camera-facing billboard; if false, a flat quad in the x/y plane (the ground). */
	bool billboard = true;
	/** Texture repeat factor; values > 1 tile the texture (used by the ground). */
	float uvScale = 1.0f;
};
} // namespace Mupfel