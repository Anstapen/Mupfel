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
		struct CellEntityPair {
			uint32_t cell = 0;
			uint32_t entity = 0;
		};
	public:
		GPUCollisionGrid(uint32_t in_num_cells_x = 64, uint32_t in_num_cells_y = 64, uint32_t in_entities_per_cell = 2048, uint32_t in_cell_size_pow = 5);
		void Init();
		uint32_t GetNumCellsX() const { return num_cells_x; }
		uint32_t GetNumCellsY() const { return num_cells_y; }
		uint32_t GetEntitiesPerCell() const { return EntitiesPerCell; }
		uint32_t GetCellSizePow() const { return cell_size_pow; }

		GPUVector<uint32_t>& GetCellCountArray() { return cell_count_array; }
		uint32_t GetCellCountArraySSBO() const { return cell_count_array.GetSSBOID(); }
		uint32_t GetCellCountIndicesSSBO() const { return cell_count_indices.GetSSBOID(); }
		uint32_t GetCellEntityArraySSBO() const { return cell_entity_array.GetSSBOID(); }
	private:
		void SetNumCells(uint32_t num_cells_x, uint32_t num_cells_y);
		void SetEntitiesPerCell(uint32_t entities_per_cell);
		void SetCellSizePow(uint32_t cell_size_pow);
		static constexpr bool is_powerof2(size_t v) {
			return v && ((v & (v - 1)) == 0);
		}
	private:
		uint32_t num_cells_x;
		uint32_t num_cells_y;
		uint32_t EntitiesPerCell;
		uint32_t cell_size_pow;
		GPUVector<uint32_t> cell_count_array;
		GPUVector<uint32_t> cell_count_indices;
		GPUVector<CellEntityPair> cell_entity_array;
	};

}



