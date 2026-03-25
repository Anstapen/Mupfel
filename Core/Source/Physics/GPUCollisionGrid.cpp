#include "GPUCollisionGrid.h"
#include <cassert>

Mupfel::GPUCollisionGrid::GPUCollisionGrid(uint32_t in_num_cells_x, uint32_t in_num_cells_y, uint32_t in_entities_per_cell, uint32_t in_cell_size_pow) :
	num_cells_x(in_num_cells_x),
	num_cells_y(in_num_cells_y),
	EntitiesPerCell(in_entities_per_cell),
	cell_size_pow(in_cell_size_pow),
	cell_count_array(),
	cell_count_indices(),
	cell_entity_array()
{
}

void Mupfel::GPUCollisionGrid::Init()
{
	/* Initialize the buffers */
	cell_count_array.resize(num_cells_x * num_cells_y, { 0 });
	cell_count_indices.resize(num_cells_x * num_cells_y, { 0 });
	cell_entity_array.resize(num_cells_x * num_cells_y * EntitiesPerCell, { 0 });
}

void Mupfel::GPUCollisionGrid::SetNumCells(uint32_t num_cells_x, uint32_t num_cells_y)
{
	this->num_cells_x = num_cells_x;
	this->num_cells_y = num_cells_y;

	/* Resize the buffers */
	cell_count_array.resize(num_cells_x * num_cells_y, { 0 });
	cell_count_indices.resize(num_cells_x * num_cells_y, { 0 });
	cell_entity_array.resize(num_cells_x * num_cells_y * EntitiesPerCell, { 0 });
}

void Mupfel::GPUCollisionGrid::SetEntitiesPerCell(uint32_t entities_per_cell)
{
	EntitiesPerCell = entities_per_cell;
	cell_entity_array.resize(num_cells_x * num_cells_y * EntitiesPerCell, { 0 });
}

void Mupfel::GPUCollisionGrid::SetCellSizePow(uint32_t cell_size_pow)
{
	this->cell_size_pow = cell_size_pow;
}
