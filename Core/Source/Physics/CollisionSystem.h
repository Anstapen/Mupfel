#pragma once
#include "Core/Coordinate.h"
#include "GPUCollisionGrid.h"
#include "ECS/Registry.h"
#include "Core/EventSystem.h"
#include <memory>
#include "ECS/GPUComponentArray.h"
#include <cstdint>
#include "GPU/GPUPrefixSum.h"

namespace Mupfel {

	class Registry;
	class EventSystem;
	
	class CollisionSystem
	{
		friend class DebugLayer;
	public:
		struct CollisionPair {
			uint32_t entity_a = 0;
			uint32_t entity_b = 0;
		};
	public:
		CollisionSystem(Registry& reg, EventSystem& evt_sys);
		void Init();
		void Update();
		void SetCellSizePow(uint32_t cell_size_pow);
		void SetNumCells(uint32_t num_cells_x, uint32_t num_cells_y);
	private:
		uint32_t WorldtoCell(Coordinate<uint32_t> c);

		void SetCallbacks();
		void SetProgramParams();
		void UpdateCellCount();
		void FillCellEntityArray();
		void GPUNarrowPhase();
		void CheckCollisions();
		void ClearBuffers();
	private:
		Registry& registry;
		EventSystem& evt_system;
		GPUCollisionGrid collision_grid;
		GPUPrefixSum prefix_sum;
		/* Shader IDs */
		uint32_t fill_cell_count_shader_id;
		uint32_t fill_cell_entity_shader_id;
		uint32_t narrow_phase_shader_id;

		/* GPU-side buffers */
		std::unique_ptr<GPUComponentArray<uint32_t>> active_entities;
		std::unique_ptr<GPUVector<CollisionPair>> colliding_entities;
		std::unique_ptr<GPUVector<uint32_t>> num_colliding_entities;
	};
}



