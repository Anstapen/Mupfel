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

namespace Mupfel {

	class Registry;
	class EventSystem;

	struct Cell {
		uint32_t startIndex;
		uint32_t count;
	};

	template<size_t num_cells_x, size_t num_cells_y, size_t entities_per_cell, size_t cell_size_pow = 6>
	struct CollisionGrid {
		static constexpr size_t NumCells = num_cells_x * num_cells_y;
		static constexpr size_t EntitiesPerCell = entities_per_cell;

		std::array<Entity, NumCells * EntitiesPerCell> entities = { 0 };
		std::array<Cell, NumCells> cells{ 0 };

	private:
		static constexpr bool is_powerof2(size_t v) {
			return v && ((v & (v - 1)) == 0);
		}
		static_assert(is_powerof2(num_cells_x));
		static_assert(is_powerof2(num_cells_y));
	};

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
		CollisionSystem(Registry& reg, EventSystem& evt_sys) : registry(reg), evt_system(evt_sys) {}
		void Init();
		void Update();
		void ToggleMultiThreading();
		bool IsMultiThreaded() const;
	private:
		void ClearOldCells(SpatialInfo& info);
		void UpdateCells(Entity e, SpatialInfo& info, Coordinate<uint32_t> cell_min, Coordinate<uint32_t> cell_max);
		void UpdateCells(Entity e, SpatialInfo& info, const CellMoveCommand& cmd);
		static uint32_t WorldtoCell(Coordinate<uint32_t> c);
		static uint32_t PointXtoCell(uint32_t x);
		static uint32_t PointYtoCell(uint32_t y);
		void CheckEntity(Entity e, uint32_t thread_index, ComponentArray<BroadCollider>& broad_collider_array, ComponentArray<SpatialInfo>& spatial_info_array);
		void SwapRemoveEntities(uint32_t cell_id, uint32_t new_entity_index, ComponentArray<SpatialInfo>& spatial_info_array);
		void RemoveEntity(const EntityDestroyedEvent& evt);
	private:
		Registry& registry;
		EventSystem& evt_system;
		bool multithreadingEnabled = false;
		static constexpr uint32_t cell_size_pow = 6;
		static constexpr uint32_t num_cells_x = 64;
		static constexpr uint32_t num_cells_y = 64;
		static constexpr uint32_t entities_per_cell = 2048;
		CollisionGrid<num_cells_x, num_cells_y, entities_per_cell, cell_size_pow> collision_grid;

		using ThreadLocalCommandBuffer = std::vector<std::vector<CellMoveCommand>>;
		ThreadLocalCommandBuffer thread_local_buffer;
		std::vector<std::future<void>> jobs;
	};
}



