#pragma once
#include "Core/Coordinate.h"
#include "ECS/Registry.h"
#include "Core/EventSystem.h"
#include "CollisionGrid.h"
#include <memory>
#include <cstdint>

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
		void UpdateCellCount();
		void FillCellEntityArray();
		void NarrowPhase();
		void CheckCollisions();
		void ClearBuffers();
	private:
		Registry& registry;
		EventSystem& evt_system;
		CollisionGrid collision_grid;
	};
}



