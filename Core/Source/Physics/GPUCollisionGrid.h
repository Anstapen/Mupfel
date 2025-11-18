#pragma once
#include "GPU/GPUVector.h"
#include "ECS/Entity.h"

namespace Mupfel {

	class CollisionSystem;
	class DebugLayer;

	struct Cell {
		uint32_t startIndex;
		uint32_t count;
	};

	class GPUCollisionGrid
	{
	friend class CollisionSystem;
	friend class DebugLayer;
	public:
		GPUCollisionGrid(uint32_t in_num_cells_x = 64, uint32_t in_num_cells_y = 64, uint32_t in_entities_per_cell = 2048, uint32_t in_cell_size_pow = 6);
		void Init();
	private:
		static constexpr bool is_powerof2(size_t v) {
			return v && ((v & (v - 1)) == 0);
		}
	private:
		const uint32_t num_cells_x;
		const uint32_t num_cells_y;
		const uint32_t EntitiesPerCell;
		const uint32_t cell_size_pow;
		GPUVector<Entity> entities;
		GPUVector<Cell> cells;
	};

}



