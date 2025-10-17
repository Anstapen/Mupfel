#pragma once
#include "Core/Coordinate.h"
#include <cstdint>

namespace Mupfel {
	
	struct BroadCollider
	{
		BroadCollider(uint32_t x_offset, uint32_t y_offset) : offset(x_offset, y_offset) {}
		Coordinate<float> min = { 0.0f, 0.0f };
		Coordinate<float> max = { 0.0f, 0.0f };
		Coordinate<uint32_t> offset;
	};

}
