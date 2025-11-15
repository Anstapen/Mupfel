#pragma once
#include "ECS/Components/SpatialInfo.h"
#include <array>
#include <vector>
#include <future>
#include "Core/Coordinate.h"
#include "Core/Debug/DebugLayer.h"
#include "ECS/Entity.h"
#include "ECS/Components/BroadCollider.h"
#include "ECS/ComponentArray.h"
#include "GPUCollisionGrid.h"

namespace Mupfel {

	class Registry;
	class EventSystem;

	struct CellMoveCommand {
		Entity e;
		std::array< uint32_t, SpatialInfo::n_memorised_cells> new_cells;
		uint32_t new_count = 0;
		Coordinate<uint32_t> new_min;
		Coordinate<uint32_t> new_max;
	};

	
	class CollisionSystem
	{
		friend class DebugLayer;
	public:
		CollisionSystem(Registry& reg, EventSystem& evt_sys);
		void Init();
		void Update();
	private:
		void ClearOldCells(Entity e, SpatialInfo info);
		static uint32_t WorldtoCell(Coordinate<uint32_t> c);
		static uint32_t PointXtoCell(uint32_t x);
		static uint32_t PointYtoCell(uint32_t y);
		void CheckEntity(Entity e, uint32_t thread_index, Registry& registry);
		void SwapRemoveEntities(uint32_t cell_id, uint32_t new_entity_index);
		void RemoveEntity(const EntityDestroyedEvent& evt);


		void SetCallbacks();
		void Join();
		void SetProgramParams();
		void UpdateCells();
		void UpdateCells(Entity e, SpatialInfo info, Coordinate<uint32_t> cell_min, Coordinate<uint32_t> cell_max);
		void CheckCollisions();
		void CheckEntityCollision(uint32_t start, uint32_t num_ents);
	private:
		Registry& registry;
		EventSystem& evt_system;
		bool multithreadingEnabled = false;
		static constexpr uint32_t cell_size_pow = 6;
		static constexpr uint32_t num_cells_x = 64;
		static constexpr uint32_t num_cells_y = 64;
		static constexpr uint32_t entities_per_cell = 2048;
		GPUCollisionGrid collision_grid;

		using ThreadLocalCommandBuffer = std::vector<std::vector<CellMoveCommand>>;
		ThreadLocalCommandBuffer thread_local_buffer;
		std::vector<std::future<void>> jobs;
	};
}



