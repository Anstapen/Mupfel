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

struct Collider {
    // general info, not used by the shader currently
	// with padding, this occupies 32 bytes
    uint type;
    uint layer;
    uint mask;
    uint flags;
    uint callback_id;
    uint padA;
    uint padB;
    uint padC;

    // Shape data, currently only circle
	// with padding, this occupies 16 bytes
    float radius;
    float padShape1;
    float padShape2;
    float padShape3;

    // Spatial data for the broadphase
	// this occupies 152 bytes
    uvec2 old_cell_min;
    uvec2 old_cell_max;

    CellIndex cell_indices[16];

    uint  num_cells;
    float bounding_box_size;
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
	uint max_colliding_ents;
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

layout(std430, binding = 6) buffer ColliderComponents {
    Collider colliders[];
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

void UpdateCellsOfEntity(uint e, uint comp_index, uvec2 cell_min, uvec2 cell_max)
{
	/* TODO: Update the min and max cells and add the entity to the cells */
	uint ref_count = 0;
	Collider collider = colliders[comp_index];
	collider.num_cells = 0;

	for (uint y = cell_min.y; y <= cell_max.y; y++)
	{
		for (uint x = cell_min.x; x <= cell_max.x; x++)
		{

			/* Add the Entity to the current cell */
			/* Calculate the index of the wanted cell in the cell array */
			uint cell_index = y * params.num_cells_x + x;

			/* Calculate the starting index of the entity array */
			uint cell_start_index = cells[cell_index].startIndex;

			uint cell_count = atomicAdd(cells[cell_index].count, 1);

			entities[cell_start_index + cell_count] = e;

			/* Update the SpatialInfo component of the entity */
			collider.cell_indices[ref_count].cell_id = cell_index;
			collider.cell_indices[ref_count].entity_id = cell_count;
			collider.num_cells++;

			ref_count++;
		}
	}
	/* Add the new Cell Boundaries to the SpatialInfo component */
	collider.old_cell_max = cell_max;
	collider.old_cell_min = cell_min;

	/* Write the new Spatial info */
	colliders[comp_index] = collider;
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
	Collider collider = colliders[sIndex];
	float collider_half = collider.bounding_box_size / 2;

	float min_x = t.pos.x - collider_half;
	float min_y = t.pos.y - collider_half;
	float max_x = t.pos.x + collider_half;
	float max_y = t.pos.y + collider_half;

	/* Update the Grid */
	uint cell_min_x = PointXtoCell(uint(floor(min_x)));
	uint cell_min_y = PointYtoCell(uint(floor(min_y)));
	uint cell_max_x = PointXtoCell(uint(floor(max_x - 1.0f)));
	uint cell_max_y = PointYtoCell(uint(floor(max_y - 1.0f)));

	uvec2 cell_min = { cell_min_x, cell_min_y };
	uvec2 cell_max = { cell_max_x,  cell_max_y };

	UpdateCellsOfEntity(e, sIndex, cell_min, cell_max);
}