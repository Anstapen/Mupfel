#version 460
#extension GL_ARB_gpu_shader_int64 : require


layout(local_size_x = 256) in;

struct CellIndex {
		uint cell_id;
		uint entity_id;
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

// --- Output: Join-Ergebnisse ---
struct ActiveEntity {
    uint e;   // Entity ID
    uint ti;  // Transform dense index
    uint si;  // SpatialInfo dense index
};

layout(std430, binding = 8) readonly buffer ProgramParam {
    ProgramParams params;
};

void main()
{
    // Clear the Cells, they get rebuilt in the next shader 
}