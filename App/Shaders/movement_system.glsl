#version 430

// Pro Entity: Position + Velocity
struct MovementEntity {
    vec2 position;
    vec2 velocity;
    vec2 acceleration;
	float rotation;
	float angular_velocity;
};

layout(std430, binding = 0) buffer EntityBuffer {
    MovementEntity entities[];
};

uniform float uDeltaTime;
uniform uint  uEntityCount;

layout(local_size_x = 256) in;

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= uEntityCount) return;

    // Check bounds here for now
    if (entities[id].position.x > 2300)
    {
        entities[id].position.x = 2299;
        entities[id].velocity.x *= -1.0;
    }

    if (entities[id].position.x < 300)
    {
        entities[id].position.x = 301;
        entities[id].velocity.x *= -1.0;
    }

    if (entities[id].position.y > 1400)
    {
        entities[id].position.y = 1399;
        entities[id].velocity.y *= -1.0;
    }

    if (entities[id].position.y < 50)
    {
        entities[id].position.y = 51;
        entities[id].velocity.y *= -1.0;
    }

    entities[id].position += entities[id].velocity * uDeltaTime;
    entities[id].rotation += entities[id].angular_velocity * uDeltaTime;
}