#version 430 core

layout (location = 0) in vec3 aPos;    // Quad-Vertex (statisch)
layout (location = 1) in vec2 aUV;     // Quad-UV (statisch)

struct MovementEntity {
    vec2 position;
    vec2 velocity;
};

layout(std430, binding = 0) buffer EntityBuffer {
    MovementEntity entities[];
};

// Instance data
//layout (location = 2) in vec2 iPos;
//layout (location = 3) in vec2 iScale;
//layout (location = 4) in vec2 iRotation;

uniform mat4 view;
uniform mat4 projection;

out vec2 vUV;

void main()
{
    uint instanceID = gl_InstanceID;
    // 2D Rotation Matrix
    //float c = cos(iRotation.x);
    //float s = sin(iRotation.x);
    //mat2 rot = mat2(c, -s, s, c);

    // Skaliere und rotiere das Vertex (aPos.xy), 
    // dann verschiebe es um iPos
    vec2 scale = vec2(32.0, 32.0);
    vec2 worldPos = (aPos.xy * scale) + entities[instanceID].position;

    // Baue finalen Clipspace-Vektor
    gl_Position = projection * view * vec4(worldPos, 0.0, 1.0);

    vUV = aUV;
}