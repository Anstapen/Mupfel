#include "GPUCollisionGrid.h"
#include <cassert>

Mupfel::GPUCollisionGrid::GPUCollisionGrid(size_t in_num_cells_x, size_t in_num_cells_y, size_t in_entities_per_cell, size_t in_cell_size_pow) :
	num_cells_x(in_num_cells_x), num_cells_y(in_num_cells_y), EntitiesPerCell(in_entities_per_cell), cell_size_pow(in_cell_size_pow)
{
	assert(is_powerof2(num_cells_x));
	assert(is_powerof2(num_cells_y));
	assert(is_powerof2(EntitiesPerCell));
}

void Mupfel::GPUCollisionGrid::Init()
{
	entities.resize(num_cells_x * num_cells_y * EntitiesPerCell, Entity());
	cells.resize(num_cells_x * num_cells_y, { 0, 0 });

	/* Fill the Cells with the correct offsets */
	uint32_t running_offset = 0;

	for (uint32_t y = 0; y < num_cells_y; y++)
	{
		for (uint32_t x = 0; x < num_cells_x; x++)
		{
			const uint32_t index = y * num_cells_x + x;

			cells[index].startIndex = running_offset;
			cells[index].count = 0;
			running_offset += EntitiesPerCell;
		}
	}
}
