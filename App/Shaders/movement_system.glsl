#version 430

// Pro Entity: Position + Velocity
struct Transform {
    vec2 pos;
	vec2 scale;
	vec2 rotation;
};

struct Velocity {
		vec2 vel;
};

struct ComponentIndices {
	uint transform_index;
	uint velocity_index;
};

layout(std430, binding = 0) buffer TransformBuffer {
    Transform transforms[];
};

layout(std430, binding = 1) buffer VelocityBuffer {
    Velocity velocities[];
};

layout(std430, binding = 2) buffer IndexBuffer {
    ComponentIndices indices[];
};

uniform float uDeltaTime;
uniform uint  uEntityCount;

layout(local_size_x = 256) in;

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= uEntityCount) return;

    uint pos_index = indices[id].transform_index;
    uint vel_index = indices[id].velocity_index;

    vec2 position = transforms[pos_index].pos;
    vec2 velocity = velocities[vel_index].vel;

    // Check bounds here for now
    if (position.x > 2300)
    {
        position.x = 2299;
        velocity.x *= -1.0;
    }

    if (position.x < 300)
    {
        position.x = 301;
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

    transforms[pos_index].pos += velocity * uDeltaTime;
    velocities[vel_index].vel = velocity;
}