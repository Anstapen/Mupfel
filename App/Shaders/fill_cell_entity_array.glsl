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

    float bounding_box_x;
    float bounding_box_y;
};

struct ProgramParams {
	uint64_t active_entities;
    uint cell_size_pow;
	uint num_cells_x;
	uint num_cells_y;
	uint _padding;
};

layout(std430, binding = 1) buffer CellOffsets {
    uint cell_offset[];
};

layout(std430, binding = 2) buffer CellEntityArray {
    CellIndex cell_entity_array[];
};


layout(std430, binding = 3) buffer TransformComponents {
    TransformData transforms[];
};

layout(std430, binding = 4) readonly buffer TransformSparse {
    uint transformSparse[];
};

layout(std430, binding = 5) readonly buffer SpatialSparse {
    uint spatialSparse[]; 
};

layout(std430, binding = 6) buffer ColliderComponents {
    Collider colliders[];
};

layout(std430, binding = 7) buffer ActiveEntities {
    uint entities[];
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

	for (uint y = cell_min.y; y <= cell_max.y; y++)
	{
		for (uint x = cell_min.x; x <= cell_max.x; x++)
		{
			/* Increase the cell_count for the current cell */
			/* Add the Entity to the current cell */
			/* Calculate the index of the wanted cell in the cell array */
			uint cell_index = y * params.num_cells_x + x;

			uint cell_array_index = atomicAdd(cell_offset[cell_index], 1);
			cell_entity_array[cell_array_index] = CellIndex(cell_index, e);
		}
	}

}

void main()
{
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= params.active_entities) return;

    // If entity is 0, ignore
	uint e = entities[idx];

    if(e == 0) return;

    uint tIndex = transformSparse[e];
    uint sIndex = spatialSparse[e];

	TransformData t = transforms[tIndex];
	Collider collider = colliders[sIndex];


	float collider_half_x = collider.bounding_box_x / 2;
	float collider_half_y = collider.bounding_box_y / 2;

	float min_x = t.pos.x - collider_half_x;
	float min_y = t.pos.y - collider_half_y;
	float max_x = t.pos.x + collider_half_x;
	float max_y = t.pos.y + collider_half_y;

	/* Update the Grid */
	uint cell_min_x = PointXtoCell(uint(floor(min_x)));
	uint cell_min_y = PointYtoCell(uint(floor(min_y)));
	uint cell_max_x = PointXtoCell(uint(floor(max_x - 1.0f)));
	uint cell_max_y = PointYtoCell(uint(floor(max_y - 1.0f)));

	uvec2 cell_min = { cell_min_x, cell_min_y };
	uvec2 cell_max = { cell_max_x,  cell_max_y };

	UpdateCellsOfEntity(e, sIndex, cell_min, cell_max);
}