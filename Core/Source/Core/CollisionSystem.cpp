#include "CollisionSystem.h"
#include <algorithm>
#include <cassert>
#include <array>
#include <thread>
#include "Application.h"

/* Needed Component types for collision detection/resolution */
#include "ECS/Components/SpatialInfo.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/BroadCollider.h"

using namespace Mupfel;

void CollisionSystem::Init()
{
	/* Fill the Cells with the correct offsets */
	uint32_t running_offset = 0;
	
	for (uint32_t y = 0; y < num_cells_y; y++)
	{
		for (uint32_t x = 0; x < num_cells_x; x++)
		{
			const uint32_t index = y * num_cells_x + x;

			collision_grid.cells[index].startIndex = running_offset;
			collision_grid.cells[index].count = 0;
			running_offset += collision_grid.EntitiesPerCell;
		}
	}

	/* Register the callback for Auto-Cleanup of destroyed entities */
	evt_system.RegisterListener<EntityDestroyedEvent>(
		[this](const EntityDestroyedEvent& evt) { RemoveEntity(evt); }
	);

	/* Scale the Thread local buffer */
	const size_t num_threads = Application::GetCurrentThreadPool().GetThreadCount();
	thread_local_buffer.resize(num_threads);

	jobs.clear();
	jobs.reserve(num_threads);

	/*
		Reserve some sane amount of memory for each Buffer.
		Let's go for 100 Move Commands per buffer.
	*/
	for (auto& buf : thread_local_buffer)
	{
		buf.reserve(100);
	}
}

void CollisionSystem::Update()
{


	/* Currently we are only interested in changed positions */
	auto& transform_array = registry.GetComponentArray<Transform>();

	auto& dirty_entities = transform_array.GetDirtyEntities();

	/* If no Entities needs updates, we can exit */
	if (dirty_entities.empty())
	{
		return;
	}

	if (multithreadingEnabled && dirty_entities.size() > 2000 * Application::GetCurrentThreadPool().GetThreadCount())
	{
		auto& thread_pool = Application::GetCurrentThreadPool();
		const uint32_t num_threads = static_cast<uint32_t>(thread_pool.GetThreadCount());

		/* Safe the component arrays */
		ComponentArray<BroadCollider>& broad_collider_array = registry.GetComponentArray<BroadCollider>();
		ComponentArray<SpatialInfo>& spatial_info_array = registry.GetComponentArray<SpatialInfo>();

		if (thread_local_buffer.size() != num_threads) {
			thread_local_buffer.clear();
			thread_local_buffer.resize(num_threads);
		}
		
		const uint32_t total_entities = static_cast<uint32_t>(dirty_entities.size());

		/* Split up the entities for the threads */
		const uint32_t chunk = (total_entities + num_threads - 1) / num_threads;

		for (uint32_t t = 0; t < num_threads; t++)
		{
			const uint32_t begin = t * chunk;
			const uint32_t end = std::min(begin + chunk, total_entities);

			if (begin >= end) continue;

			/* Push a new job */
			jobs.push_back(thread_pool.Enqueue([&, t, begin, end]() {
				for (uint32_t i = begin; i < end; i++)
				{
					CheckEntity(dirty_entities[i], t, broad_collider_array, spatial_info_array);
				}
			}));
		}

		/* Wait for all jobs to finish */
		for (auto& job : jobs)
		{
			job.get();
		}

		/* Jobs are done for this frame, clear the buffer */
		jobs.clear();

		/* Resolve Commands */
		ComponentArray<SpatialInfo>& spatial_array = registry.GetComponentArray<SpatialInfo>();
		for (auto& cmd_buffer : thread_local_buffer)
		{
			for (auto& cmd : cmd_buffer)
			{
				auto& spatial_info = spatial_array.Get(cmd.e);
				ClearOldCells(spatial_info);
				UpdateCells(cmd.e, spatial_info, cmd);
			}
			cmd_buffer.clear();
		}

	}
	else
	{
		ComponentArray<SpatialInfo>& spatial_array = registry.GetComponentArray<SpatialInfo>();
		ComponentArray<BroadCollider>& broad_collider_array = registry.GetComponentArray<BroadCollider>();
		for (Entity e : dirty_entities)
		{
			/* For now, we are only interested in entities that have the BroadCollider and SpatialInfo Component */

			if (!broad_collider_array.Has(e) || !spatial_array.Has(e))
			{
				continue;
			}

			auto& broad_collider = broad_collider_array.Get(e);
			auto& spatial_info = spatial_array.Get(e);

			/* Update the Grid */
			uint32_t cell_min_x = PointXtoCell(static_cast<uint32_t>(std::floor(broad_collider.min.x)));
			uint32_t cell_min_y = PointYtoCell(static_cast<uint32_t>(std::floor(broad_collider.min.y)));
			uint32_t cell_max_x = PointXtoCell(static_cast<uint32_t>(std::floor(broad_collider.max.x - 1.0f)));
			uint32_t cell_max_y = PointYtoCell(static_cast<uint32_t>(std::floor(broad_collider.max.y - 1.0f)));

			Coordinate<uint32_t> cell_min = { cell_min_x, cell_min_y };
			Coordinate<uint32_t> cell_max = { cell_max_x,  cell_max_y };

			if (spatial_info.old_cell_max == cell_max && spatial_info.old_cell_min == cell_min)
			{
				continue;
			}

			ClearOldCells(spatial_info);

			UpdateCells(e, spatial_info, cell_min, cell_max);
		}
	}

	transform_array.ClearDirtyList();
}

void Mupfel::CollisionSystem::ToggleMultiThreading()
{
	multithreadingEnabled = !multithreadingEnabled;
}

bool Mupfel::CollisionSystem::IsMultiThreaded() const
{
	return multithreadingEnabled;
}

void Mupfel::CollisionSystem::ClearOldCells(SpatialInfo& info)
{
	if (info.num_cells == 0)
	{
		/* Nothing to do */
		return;
	}

	ComponentArray<SpatialInfo>& spatial_info_array = registry.GetComponentArray<SpatialInfo>();

	uint32_t ref_count = 0;
	for (uint32_t y = info.old_cell_min.y; y <= info.old_cell_max.y; y++)
	{
		for (uint32_t x = info.old_cell_min.x; x <= info.old_cell_max.x; x++)
		{
			assert(ref_count < info.num_cells);

			/* Calculate the index of the wanted cell in the cell array */
			uint32_t cell_index = y * num_cells_x + x;

			/* if the values of old_cell_min and old_cell_max are correct, these two values should match */
			assert(cell_index == info.refs[ref_count].cell_id);

			/* The entity component holds the index into the entity array of the current cell */
			uint32_t entity_index = info.refs[ref_count].entity_id;
			/* Swap-Remove */
			SwapRemoveEntities(cell_index, entity_index, spatial_info_array);

			ref_count++;
		}
	}

	/* Removed Entity from the Cells */
	info.num_cells = 0;
}

void Mupfel::CollisionSystem::UpdateCells(Entity e, SpatialInfo& info, Coordinate<uint32_t> cell_min, Coordinate<uint32_t> cell_max)
{
	/* TODO: Update the min and max cells and add the entity to the cells */
	uint32_t ref_count = 0;
	for (uint32_t y = cell_min.y; y <= cell_max.y; y++)
	{
		for (uint32_t x = cell_min.x; x <= cell_max.x; x++)
		{
			/* An Entity should never overlap with more than 16 cells */
			assert(ref_count < info.refs.size());

			/* Add the Entity to the current cell */
			/* Calculate the index of the wanted cell in the cell array */
			uint32_t cell_index = y * num_cells_x + x;

			/* Calculate the starting index of the entity array */
			uint32_t cell_start_index = collision_grid.cells[cell_index].startIndex;

			uint32_t cell_count = collision_grid.cells[cell_index].count;

			assert(cell_count < collision_grid.EntitiesPerCell);

			collision_grid.entities[cell_start_index + cell_count] = e.Index();

			/* Update the SpatialInfo component of the entity */
			info.refs[ref_count].cell_id = cell_index;
			info.refs[ref_count].entity_id = cell_count;
			info.num_cells++;

			/* Increment the cell entity counter */
			collision_grid.cells[cell_index].count++;

			ref_count++;

			assert(info.num_cells == ref_count);
		}
	}
	/* Add the new Cell Boundaries to the SpatialInfo component */
	info.old_cell_max = cell_max;
	info.old_cell_min = cell_min;
}

void Mupfel::CollisionSystem::UpdateCells(Entity e, SpatialInfo& info, const CellMoveCommand& cmd)
{
	for (uint32_t i = 0; i < cmd.new_count; i++)
	{
		uint32_t cell_index = cmd.new_cells[i];

		/* Calculate the starting index of the entity array */
		uint32_t cell_start_index = collision_grid.cells[cell_index].startIndex;

		uint32_t cell_count = collision_grid.cells[cell_index].count;

		assert(cell_count < collision_grid.EntitiesPerCell);

		collision_grid.entities[cell_start_index + cell_count] = e.Index();

		/* Update the SpatialInfo component of the entity */
		info.refs[i].cell_id = cell_index;
		info.refs[i].entity_id = cell_count;
		info.num_cells++;

		/* Increment the cell entity counter */
		collision_grid.cells[cell_index].count++;

		assert(info.num_cells == i + 1);
	}
	info.old_cell_max = cmd.new_max;
	info.old_cell_min = cmd.new_min;
}

uint32_t Mupfel::CollisionSystem::WorldtoCell(Coordinate<uint32_t> c)
{
	uint32_t cell_x = c.x >> cell_size_pow;
	uint32_t cell_y = c.y >> cell_size_pow;

	cell_x = std::min(cell_x, num_cells_x - 1);
	cell_y = std::min(cell_y, num_cells_y - 1);

	return cell_y * num_cells_x + cell_x;
}

uint32_t Mupfel::CollisionSystem::PointXtoCell(uint32_t x)
{
	uint32_t cell_x = x >> cell_size_pow;
	return std::min(cell_x, num_cells_x - 1);
}

uint32_t Mupfel::CollisionSystem::PointYtoCell(uint32_t y)
{
	uint32_t cell_y = y >> cell_size_pow;
	return std::min(cell_y, num_cells_y - 1);
}

void Mupfel::CollisionSystem::CheckEntity(Entity e, uint32_t thread_index, ComponentArray<BroadCollider>& broad_collider_array, ComponentArray<SpatialInfo>& spatial_info_array)
{
	/* For now, we are only interested in entities that have the BroadCollider and SpatialInfo Component */

	if (!broad_collider_array.Has(e) || !spatial_info_array.Has(e))
	{
		return;
	}

	auto& broad_collider = broad_collider_array.Get(e);
	auto& spatial_info = spatial_info_array.Get(e);
	/* Update the Grid */
	uint32_t cell_min_x = PointXtoCell(static_cast<uint32_t>(std::floor(broad_collider.min.x)));
	uint32_t cell_min_y = PointYtoCell(static_cast<uint32_t>(std::floor(broad_collider.min.y)));
	uint32_t cell_max_x = PointXtoCell(static_cast<uint32_t>(std::floor(broad_collider.max.x - 1.0f)));
	uint32_t cell_max_y = PointYtoCell(static_cast<uint32_t>(std::floor(broad_collider.max.y - 1.0f)));

	Coordinate<uint32_t> cell_min = { cell_min_x, cell_min_y };
	Coordinate<uint32_t> cell_max = { cell_max_x,  cell_max_y };

	if (spatial_info.old_cell_max == cell_max && spatial_info.old_cell_min == cell_min)
	{
		return;
	}

	/* Cells of the Entity have changed, update them */
	CellMoveCommand cmd{};
	cmd.e = e;
	
	/* Enter the new Cells into the command */
	uint8_t count = 0;
	for (uint32_t y = cell_min.y; y <= cell_max.y && count < cmd.new_cells.size(); ++y)
	{
		for (uint32_t x = cell_min.x; x <= cell_max.x && count < cmd.new_cells.size(); ++x)
		{	
			cmd.new_cells[count++] = y * num_cells_x + x;
		}
	}

	cmd.new_count = count;

	/* Add the new Cell Boundaries to the command */
	cmd.new_max = cell_max;
	cmd.new_min = cell_min;

	thread_local_buffer[thread_index].push_back(std::move(cmd));
}

void Mupfel::CollisionSystem::SwapRemoveEntities(uint32_t cell_id, uint32_t new_entity_index, ComponentArray<SpatialInfo>& spatial_info_array)
{
	/* Calculate the starting index of the entity array */
	uint32_t cell_start_index = collision_grid.cells[cell_id].startIndex;

	uint32_t cell_count = collision_grid.cells[cell_id].count;

	uint32_t entity_location = cell_start_index + new_entity_index;

	uint32_t last_entity = cell_start_index + cell_count - 1;
	assert(entity_location >= cell_start_index &&
		entity_location < cell_start_index + cell_count);


	collision_grid.entities[entity_location] = collision_grid.entities[last_entity];
	collision_grid.entities[last_entity] = 0;
	collision_grid.cells[cell_id].count -= 1;

	if (entity_location != last_entity)
	{
		/* Update the SpatialInfo component of the swapped entity */
		/* Removal of destroyed entities is done asynchronously */
		Entity entity_to_be_updated = collision_grid.entities[entity_location];
		if (!spatial_info_array.Has(entity_to_be_updated))
		{
			return;
		}
		auto& spatial_comp = spatial_info_array.Get(entity_to_be_updated);
		for (uint32_t i = 0; i < spatial_comp.num_cells; i++)
		{
			/* We need to find the cell with the correct index */
			if (cell_id == spatial_comp.refs[i].cell_id)
			{
				spatial_comp.refs[i].entity_id = new_entity_index;
				break;
			}
		}
	}
}

void Mupfel::CollisionSystem::RemoveEntity(const EntityDestroyedEvent& evt)
{
	Entity removed_entity = evt.e;

	ComponentArray<SpatialInfo>& spatial_info_array = registry.GetComponentArray<SpatialInfo>();

	/* Only entities which have a SpatialInfo component are of interest */
	if (!spatial_info_array.Has(removed_entity))
	{
		return;
	}
	auto& spatial_info = spatial_info_array.Get(removed_entity);

	ClearOldCells(spatial_info);

}
