#version 330 core

layout (location = 0) in vec3 aPos;    // Quad-Vertex (statisch)
layout (location = 1) in vec2 aUV;     // Quad-UV (statisch)

// Instance data
layout (location = 2) in vec2 iPos;
layout (location = 3) in vec2 iScale;
layout (location = 4) in float iRotation;

uniform mat4 view;
uniform mat4 projection;

out vec2 vUV;

void main()
{
    // 2D Rotation Matrix
    float c = cos(iRotation);
    float s = sin(iRotation);
    mat2 rot = mat2(c, -s, s, c);

    // Skaliere und rotiere das Vertex (aPos.xy), 
    // dann verschiebe es um iPos
    vec2 worldPos = rot * (aPos.xy * iScale) + iPos;

    // Baue finalen Clipspace-Vektor
    gl_Position = projection * view * vec4(worldPos, 0.0, 1.0);

    vUV = aUV;
}