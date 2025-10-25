#version 430

// Pro Entity: Position + Velocity
struct MovementEntity {
    vec2 position;
    vec2 velocity;
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
    if (entities[id].position.x > 2000)
    {
        entities[id].position.x = 1950;
        entities[id].velocity.x *= -1.0;
    }

    if (entities[id].position.x < 0)
    {
        entities[id].position.x = 50;
        entities[id].velocity.x *= -1.0;
    }

    if (entities[id].position.y > 1000)
    {
        entities[id].position.y = 950;
        entities[id].velocity.y *= -1.0;
    }

    if (entities[id].position.y < 0)
    {
        entities[id].position.y = 50;
        entities[id].velocity.y *= -1.0;
    }

    entities[id].position += entities[id].velocity * uDeltaTime;
}