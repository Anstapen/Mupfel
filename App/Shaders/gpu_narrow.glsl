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

    float bounding_box_size;
};

struct CollisionPair {
	uint entity_a;
	uint entity_b;
};

struct ProgramParams {
	uint64_t active_entities;
    uint cell_size_pow;
	uint num_cells_x;
	uint num_cells_y;
    uint _padding;
};

layout(std430, binding = 1) readonly buffer CellCountIndices {
    uint cell_index[];
};

layout(std430, binding = 2) buffer CellEntityArray {
    CellIndex cells[];
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

layout(std430, binding = 8) readonly buffer ProgramParam {
    ProgramParams params;
};

layout(std430, binding = 9) buffer CollidingEntities {
    CollisionPair collision_pairs[];
};

layout(std430, binding = 10) buffer num_colliding_entities {
    uint num;
};


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

    if (idx >= (params.num_cells_x * params.num_cells_y - 1)) return;
    uint cell_count = cell_index[idx + 1] - cell_index[idx];
    uint cell_start = cell_index[idx];

    for (uint first = 0; first < cell_count; first++)
	{
		for (uint second = first + 1; second < cell_count; second++)
		{
            uint entity_a = cells[cell_start + first].entity_id;
            uint entity_b = cells[cell_start + second].entity_id;

            if(CollisionPossible(entity_a, entity_b))
            {
                uint collision_idx = atomicAdd(num, 1);
                collision_pairs[collision_idx] = CollisionPair(entity_a, entity_b);
            }
        }
    }
}