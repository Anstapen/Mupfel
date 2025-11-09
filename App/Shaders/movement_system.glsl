#version 460
#extension GL_ARB_gpu_shader_int64 : require


layout(local_size_x = 256) in;

struct TransformData {
    vec2 pos;
    vec2 scale;
    vec2 rotation; // y component of rotation is padding on cpu side! dont use!
};

struct VelocityData {
    vec2 vel;
    vec2 angular; // y component of angular is padding on cpu side! dont use!
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

layout(std430, binding = 6) buffer VelocityComponents {
    VelocityData velocities[];
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

void main()
{
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= params.active_entities) return;

    // If entity is 0, ignore
    if(pairs[idx].e == 0) return;

    uint tIndex = pairs[idx].ti;
    uint vIndex = pairs[idx].vi;

    vec2 position = transforms[tIndex].pos;
    vec2 velocity = velocities[vIndex].vel;

    float angular_velocity = velocities[vIndex].angular.x;


    // Check bounds here for now
    if (position.x > 2300)
    {
        position.x = 2299;
        velocity.x *= -1.0;
    }

    if (position.x < 500)
    {
        position.x = 501;
        velocity.x *= -1.0;
    }

    if (position.y > 1400)
    {
        position.y = 1399;
        velocity.y *= -1.0;
    }

    if (position.y < 50)
    {
        position.y = 51;
        velocity.y *= -1.0;
    }
    velocities[vIndex].vel = velocity;
    transforms[tIndex].pos = position;

    transforms[tIndex].rotation.x += angular_velocity * params.delta_time;

    transforms[tIndex].pos += velocities[vIndex].vel * params.delta_time;
}