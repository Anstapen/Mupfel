#pragma once
#include "Core/Coordinate.h"
#include <cstdint>
#include <array>

namespace Mupfel {

	struct CellIndex {
		uint32_t cell_id;
		uint32_t entity_id;
	};

	struct SpatialInfo
	{
		Coordinate<uint32_t> old_cell_min = { 0,0 };
		Coordinate<uint32_t> old_cell_max = { 0,0 };
		uint32_t num_cells = 0;
		std::array<CellIndex, 16> refs = { 0, 0 };
	};

}
