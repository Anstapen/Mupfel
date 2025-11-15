#version 460
#extension GL_ARB_gpu_shader_int64 : require


layout(local_size_x = 256) in;

struct TransformData {
    vec3 pos;
    vec2 scale;
    float rotation;
};

struct SpatialInfo {
    vec3 vel;
    float angular; 
};

struct ProgramParams {
	uint64_t component_mask;
	uint64_t active_entities;
	uint64_t entities_changed;
    uint64_t entities_deleted;
	float delta_time;
};


layout(std430, binding = 3) buffer TransformComponents {
    TransformData transforms[];
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

void main()
{
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= params.active_entities) return;

    // If entity is 0, ignore
    if(pairs[idx].e == 0) return;

    uint tIndex = pairs[idx].ti;
    uint sIndex = pairs[idx].si;
}