#version 460
#extension GL_ARB_gpu_shader_int64 : require

// This shader is responsible to update the buffer of entities
// that need a movement update (Entities with a Transform and Velocity component)
// This shader gets called, when there were Transform or Velocity components removed
// from entities this frame


layout(local_size_x = 256) in;

struct ProgramParams {
	uint64_t component_mask;
	uint64_t active_entities;
	uint64_t entities_added;
    uint64_t entities_deleted;
};

layout(std430, binding = 0) readonly buffer EntitySignatures {
    uint64_t signatures[];
};

layout(std430, binding = 1) readonly buffer TransformSparse {
    uint transformSparse[];
};

layout(std430, binding = 4) readonly buffer SpatialSparse {
    uint spatialSparse[]; 
};

layout(std430, binding = 10) readonly buffer AddedEntities {
    uint added_entities[];
};

layout(std430, binding = 11) readonly buffer DeletedEntities {
    uint deleted_entities[];
};

struct ActiveEntity {
    uint e;   // Entity ID
    uint ti;  // Transform dense index
    uint si;  // Spatial dense index
};

layout(std430, binding = 7) buffer ActiveEntities {
    ActiveEntity pairs[];
};

layout(std430, binding = 8) readonly buffer ProgramParam {
    ProgramParams params;
};

void main() {
    uint idx = gl_GlobalInvocationID.x;
    
    if(idx < params.entities_deleted)
    {
        // For now we iterate through the active entity list (O(n) complexity)
        for(uint i = 0; i < params.active_entities; i++)
        {
            if(pairs[i].e == deleted_entities[idx])
            {
                // Found the entity to be deleted, just clear the entry and add the index to the free list
                pairs[i] = ActiveEntity(0, 0, 0);
            }
        }
    }

    
    if(idx < params.entities_added)
    {
        uint entityID = added_entities[idx];
        uint tIndex = transformSparse[entityID];
        uint sIndex = spatialSparse[entityID];

        pairs[tIndex] = ActiveEntity(entityID, tIndex, sIndex);
    }

}