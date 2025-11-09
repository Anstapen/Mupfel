#version 460
#extension GL_ARB_gpu_shader_int64 : require

// This shader fills a SSBO with the necessary data to process entities which have both a transform and veloctiy component

layout(local_size_x = 256) in;

struct TransformData {
    vec2 pos;
    vec2 scale;
    vec2 rotation; // y component of rotation is padding on cpu side! dont use!
};

struct VelocityData {
    vec2 vel;
};

struct ProgramParams {
	uint64_t component_mask;
	uint64_t active_entities;
	uint64_t entities_added;
    uint64_t entities_deleted;
	float delta_time;
};

// --- Entity Signaturen (128 Bit pro Entity) ---
layout(std430, binding = 0) readonly buffer EntitySignatures {
    uint64_t signatures[];
};

// --- Transform Arrays ---
layout(std430, binding = 1) readonly buffer TransformSparse {
    uint transformSparse[];
};

layout(std430, binding = 2) readonly buffer TransformDense {
    uint transformDense[];
};

layout(std430, binding = 3) readonly buffer TransformComponents {
    TransformData transforms[];
};

layout(std430, binding = 4) readonly buffer VelocitySparse {
    uint velocitySparse[]; 
};

layout(std430, binding = 5) readonly buffer VelocityDense {
    uint velocityDense[]; 
};

layout(std430, binding = 6) readonly buffer VelocityComponents {
    VelocityData velocities[];
};

layout(std430, binding = 10) readonly buffer AddedEntities {
    uint added_entities[];
};

layout(std430, binding = 11) readonly buffer DeletedEntities {
    uint deleted_entities[];
};

// --- Output: Join-Ergebnisse ---
struct ActiveEntity {
    uint e;   // Entity ID
    uint ti;  // Transform dense index
    uint vi;  // Velocity dense index
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
        uint vIndex = velocitySparse[entityID];

        pairs[tIndex] = ActiveEntity(entityID, tIndex, vIndex);
    }
    

}