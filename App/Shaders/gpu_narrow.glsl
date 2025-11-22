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

struct CollisionPair {
	uint entity_a;
	uint entity_b;
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

layout(std430, binding = 3) readonly buffer TransformSparse {
    uint transformSparse[];
};

layout(std430, binding = 4) buffer TransformComponents {
    TransformData transforms[];
};

layout(std430, binding = 5) readonly buffer ColliderSparse {
    uint colliderSparse[]; 
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

layout(std430, binding = 9) buffer CollidingEntities {
    CollisionPair collision_pairs[];
};

layout(std430, binding = 10) buffer num_colliding_entities {
    uint num;
};

uint firstMatchingCell(uint entity_a, uint entity_b)
{
    Collider c_a = colliders[colliderSparse[entity_a]];
    Collider c_b = colliders[colliderSparse[entity_b]];
    
    for(uint i = 0; i < c_a.num_cells; i++)
    {
        for(uint j = 0; j < c_b.num_cells; j++)
        {
            if(c_a.cell_indices[i].cell_id == c_b.cell_indices[j].cell_id)
            {
                return c_a.cell_indices[i].cell_id;
            }
        }
    }
    
    return 0;
}

bool CollisionPossible(uint entity_a, uint entity_b)
{
    vec2 t_a = transforms[transformSparse[entity_a]].pos.xy;
    vec2 t_b = transforms[transformSparse[entity_b]].pos.xy;
    
    Collider c_a = colliders[colliderSparse[entity_a]];
    Collider c_b = colliders[colliderSparse[entity_b]];

    float a_x = t_a.x - c_a.bounding_box_size / 2;
    float a_y = t_a.y - c_a.bounding_box_size / 2;

    float b_x = t_b.x - c_b.bounding_box_size / 2;
    float b_y = t_b.y - c_b.bounding_box_size / 2;

    bool collisionX = ((a_x + c_a.bounding_box_size) >= b_x) && ((b_x + c_b.bounding_box_size) >= a_x);
    bool collisionY = ((a_y + c_a.bounding_box_size) >= b_y) && ((b_y + c_b.bounding_box_size) >= a_y);

    return collisionX && collisionY;
}

void main()
{
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= (params.num_cells_x * params.num_cells_y)) return;

    uint cell_count = cells[idx].count;
    uint cell_start = cells[idx].startIndex;

    if(cell_count < 2) return;

    for (uint first = 0; first < cell_count; first++)
	{
		for (uint second = first + 1; second < cell_count; second++)
		{
            uint entity_a = entities[cell_start + first];
            uint entity_b = entities[cell_start + second];

            if(CollisionPossible(entity_a, entity_b))
            {
                uint first_colliding_cell = firstMatchingCell(entity_a, entity_b);
                if(first_colliding_cell == idx)
                {
                    uint idx = atomicAdd(num, 1);
                    collision_pairs[idx] = CollisionPair(entity_a, entity_b);
                }
                
            }
        }
    }

}