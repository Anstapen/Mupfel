#include "CollisionSystem.h"
#include <algorithm>
#include <cassert>

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
}

void CollisionSystem::Update()
{


	/* Currently we are only interested in changed positions */
	auto& transform_array = registry.GetComponentArray<Transform>();

	auto& dirty_entities = transform_array.GetDirtyEntities();

	for (Entity e : dirty_entities)
	{
		/* For now, we are only interested in entities that have the BroadCollider Component */

		if (!registry.HasComponent<BroadCollider>(e) || !registry.HasComponent<SpatialInfo>(e))
		{
			continue;
		}

		auto& broad_collider = registry.GetComponent<BroadCollider>(e);
		auto& spatial_info = registry.GetComponent<SpatialInfo>(e);
		/* Update the Grid */
		Coordinate<uint32_t> cell_min = { PointXtoCell(broad_collider.min.x), PointYtoCell(broad_collider.min.y) };
		Coordinate<uint32_t> cell_max = { PointXtoCell(broad_collider.max.x - 1), PointYtoCell(broad_collider.max.y - 1) };

		if (spatial_info.old_cell_max == cell_max && spatial_info.old_cell_min == cell_min)
		{
			continue;
		}

		/* Entity entered new Cells */
		/* Delete entity from old cells */
		

		ClearOldCells(spatial_info);
		
		UpdateCells(e, spatial_info, cell_min, cell_max);
	}

	transform_array.ClearDirtyList();
}

void Mupfel::CollisionSystem::ClearOldCells(SpatialInfo& info)
{
	if (info.num_cells == 0)
	{
		/* Nothing to do */
		return;
	}

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
			SwapRemoveEntities(cell_index, entity_index);

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

void Mupfel::CollisionSystem::SwapRemoveEntities(uint32_t cell_id, uint32_t new_entity_index)
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
		if (!registry.HasComponent<SpatialInfo>(entity_to_be_updated))
		{
			return;
		}
		auto& spatial_comp = registry.GetComponent<SpatialInfo>(entity_to_be_updated);
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

	/* Only entities which have a SpatialInfo component are of interest */
	if (!registry.HasComponent<SpatialInfo>(removed_entity))
	{
		return;
	}
	auto& spatial_info = registry.GetComponent<SpatialInfo>(removed_entity);

	ClearOldCells(spatial_info);

}
