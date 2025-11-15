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
		static constexpr uint32_t n_memorised_cells = 16;
		float collider_size = 16.0f;
		Coordinate<uint32_t> old_cell_min = { 0,0 };
		Coordinate<uint32_t> old_cell_max = { 0,0 };
		uint32_t num_cells = 0;
		std::array<CellIndex, n_memorised_cells> refs = { 0, 0 };
	};

}
