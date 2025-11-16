#version 460
#extension GL_ARB_gpu_shader_int64 : require


layout(local_size_x = 256) in;

struct TransformData {
    vec3 pos;
    vec2 scale;
    float rotation;
};

struct CellIndex {
		uint cell_id;
		uint entity_id;
};

struct SpatialInfo {
    float collider_size;
	uvec2 old_cell_min;
	uvec2 old_cell_max;
    uint num_cells;
    CellIndex cell_indices[16];
};

struct Cell {
	uint startIndex;
	uint count;
};

struct ProgramParams {
	uint64_t component_mask;
	uint64_t active_entities;
	uint64_t entities_added;
    uint64_t entities_deleted;
    uint cell_size_pow;
	uint num_cells_x;
	uint num_cells_y;
	uint entities_per_cell;
};

layout(std430, binding = 1) buffer Cells {
    Cell cells[];
};

layout(std430, binding = 2) buffer CellEntities {
    uint entities[];
};

layout(std430, binding = 3) buffer TransformComponents {
    TransformData transforms[];
};

layout(std430, binding = 4) readonly buffer SpatialSparse {
    uint spatialSparse[]; 
};

layout(std430, binding = 6) buffer SpatialInfoComponents {
    SpatialInfo spatials[];
};

// --- Output: Join-Ergebnisse ---
struct ActiveEntity {
    uint e;   // Entity ID
    uint ti;  // Transform dense index
    uint si;  // SpatialInfo dense index
};

layout(std430, binding = 7) buffer ActiveEntities {
    ActiveEntity pairs[];
};

layout(std430, binding = 8) readonly buffer ProgramParam {
    ProgramParams params;
};

uint PointXtoCell(uint x)
{
	uint cell_x = x >> params.cell_size_pow;
	return min(cell_x, params.num_cells_x - 1);
}

uint PointYtoCell(uint y)
{
	uint cell_y = y >>  params.cell_size_pow;
	return min(cell_y, params.num_cells_y - 1);
}

void SwapRemoveEntities(uint cell_id, uint new_entity_index)
{
	/* Calculate the starting index of the entity array */
	uint cell_start_index = cells[cell_id].startIndex;

	uint cell_count = cells[cell_id].count;

	uint entity_location = cell_start_index + new_entity_index;

	uint last_entity = cell_start_index + cell_count - 1;

	entities[entity_location] = entities[last_entity];
	entities[last_entity] = 0;
	cells[cell_id].count -= 1;

	if (entity_location != last_entity)
	{
		/* Update the SpatialInfo component of the swapped entity */
		/* Removal of destroyed entities is done asynchronously */
		uint entity_to_be_updated = entities[entity_location];
		uint comp_index = spatialSparse[entity_to_be_updated];
		if (comp_index == 0xFFFFFFFF)
		{
			return;
		}
		SpatialInfo s = spatials[comp_index];
		for (uint i = 0; i < s.num_cells; i++)
		{
			/* We need to find the cell with the correct index */
			if (cell_id == s.cell_indices[i].cell_id)
			{
				s.cell_indices[i].entity_id = new_entity_index;
				break;
			}
		}
	}
}

void ClearOldCells(uint e, uint comp_index)
{
	SpatialInfo info = spatials[comp_index];
	if (info.num_cells == 0)
	{
		/* Nothing to do */
		return;
	}

	uint ref_count = 0;

	for (uint y = info.old_cell_min.y; y <= info.old_cell_max.y; y++)
	{
		for (uint x = info.old_cell_min.x; x <= info.old_cell_max.x; x++)
		{

			/* Calculate the index of the wanted cell in the cell array */
			uint cell_index = y * params.num_cells_x + x;

			/* The entity component holds the index into the entity array of the current cell */
			uint entity_index = info.cell_indices[ref_count].entity_id;
			/* Swap-Remove */
			SwapRemoveEntities(cell_index, entity_index);

			ref_count++;
		}
	}

	/* Removed Entity from the Cells */
	info.num_cells = 0;
	spatials[comp_index] = info;
}

void UpdateCellsOfEntity(uint e, uint comp_index, uvec2 cell_min, uvec2 cell_max)
{
	/* TODO: Update the min and max cells and add the entity to the cells */
	uint ref_count = 0;
	SpatialInfo info = spatials[comp_index];
	info.num_cells = 0;

	for (uint y = cell_min.y; y <= cell_max.y; y++)
	{
		for (uint x = cell_min.x; x <= cell_max.x; x++)
		{

			/* Add the Entity to the current cell */
			/* Calculate the index of the wanted cell in the cell array */
			uint cell_index = y * params.num_cells_x + x;

			/* Calculate the starting index of the entity array */
			uint cell_start_index = cells[cell_index].startIndex;

			uint cell_count = cells[cell_index].count;

			entities[cell_start_index + cell_count] = e;

			/* Update the SpatialInfo component of the entity */
			info.cell_indices[ref_count].cell_id = cell_index;
			info.cell_indices[ref_count].entity_id = cell_count;
			info.num_cells++;

			/* Increment the cell entity counter */
			cells[cell_index].count++;

			ref_count++;
		}
	}
	/* Add the new Cell Boundaries to the SpatialInfo component */
	info.old_cell_max = cell_max;
	info.old_cell_min = cell_min;

	/* Write the new Spatial info */
	spatials[comp_index] = info;
}

void main()
{
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= params.active_entities) return;

    // If entity is 0, ignore
	uint e = pairs[idx].e;

    if(e == 0) return;

    uint tIndex = pairs[idx].ti;
    uint sIndex = pairs[idx].si;

	TransformData t = transforms[tIndex];
	SpatialInfo s = spatials[sIndex];

	float min_x = t.pos.x - s.collider_size;
	float min_y = t.pos.y - s.collider_size;
	float max_x = t.pos.x + s.collider_size;
	float max_y = t.pos.y + s.collider_size;

	/* Update the Grid */
	uint cell_min_x = PointXtoCell(uint(floor(min_x)));
	uint cell_min_y = PointYtoCell(uint(floor(min_y)));
	uint cell_max_x = PointXtoCell(uint(floor(max_x - 1.0f)));
	uint cell_max_y = PointYtoCell(uint(floor(max_y - 1.0f)));

	uvec2 cell_min = { cell_min_x, cell_min_y };
	uvec2 cell_max = { cell_max_x,  cell_max_y };

	if (s.old_cell_max == cell_max && s.old_cell_min == cell_min)
	{
		return;
	}

	ClearOldCells(e, sIndex);

	UpdateCellsOfEntity(e, sIndex, cell_min, cell_max);
}