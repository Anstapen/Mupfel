#version 460
#extension GL_ARB_gpu_shader_int64 : require


layout(local_size_x = 256) in;

struct TransformData {
    vec3 pos;
    vec2 scale;
    float rotation;
};

struct VelocityData {
    vec3 vel;
    float angular;
    float initial_acceleration;
    float acceleration_decay;
    float friction;
    float _pad0;
};

struct ProgramParams {
	uint64_t active_entities;
	float delta_time;
    float _padding;
};

layout(std430, binding = 3) buffer TransformComponents {
    TransformData transforms[];
};

layout(std430, binding = 4) readonly buffer TransformSparse {
    uint transformSparse[];
};

layout(std430, binding = 5) readonly buffer VelocitySparse {
    uint velocitySparse[];
};

layout(std430, binding = 6) buffer VelocityComponents {
    VelocityData velocities[];
};

layout(std430, binding = 7) readonly buffer ActiveEntities {
    uint enitity[];
};

layout(std430, binding = 8) readonly buffer ProgramParam {
    ProgramParams params;
};

void main()
{
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= params.active_entities) return;

    uint e = enitity[idx];

    // If entity is 0, ignore
    if(e == 0) return;

    uint tIndex = transformSparse[e];
    uint vIndex = velocitySparse[e];

    VelocityData velo = velocities[vIndex];

    // Reduce the acceleration
    if(velo.initial_acceleration > 0)
    {
        velo.initial_acceleration = clamp(velo.initial_acceleration - (velo.acceleration_decay * params.delta_time), 0, velo.initial_acceleration);
    }

    // apply the acceleration
    //velo.vel = velo.vel + (velo.initial_acceleration * params.delta_time);

    float speed = length(velo.vel);

    if (speed > 0.001)
    {
        vec2 dir = velo.vel.xy / speed;    // normalize
        speed += velo.initial_acceleration * params.delta_time;
        speed -= ((velo.friction / 100) * speed) * params.delta_time;
        clamp(speed, 0, speed);
        velo.vel.xy = dir * speed;
    }
    else
    {
        velo.vel = vec3(0);
    }

    // apply friction
    //velo.vel = velo.vel - (velo.friction * params.delta_time);

    velocities[vIndex] = velo;

    transforms[tIndex].rotation += velocities[vIndex].angular * params.delta_time;

    transforms[tIndex].pos += velo.vel * params.delta_time;
}