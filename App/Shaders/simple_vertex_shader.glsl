#version 430 core

layout (location = 0) in vec3 aPos;    // Quad-Vertex (statisch)
layout (location = 1) in vec2 aUV;     // Quad-UV (statisch)

struct Transform {
    vec2 pos;
	vec2 scale;
	vec2 rotation;
};

struct Indices {
    uint transform_index;
    uint texture_index;
};

layout(std430, binding = 0) readonly buffer TransformBuffer {
    Transform transforms[];
};

layout(std430, binding = 1) readonly buffer IndexBuffer {
    Indices indices[];
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

    uint transform_index = indices[instanceID].transform_index;
    //uint texture_index = indices[instanceID].texture_index;
    
    vec2 rotation = transforms[transform_index].rotation;
    vec2 position = transforms[transform_index].pos;
    vec2 scale = transforms[transform_index].scale;
    // 2D Rotation Matrix
    float c = cos(rotation.x);
    float s = sin(rotation.x);
    mat2 rot = mat2(c, -s, s, c);

    // Skaliere und rotiere das Vertex (aPos.xy), 
    // dann verschiebe es um iPos
    
    vec2 worldPos = rot * (aPos.xy * scale) + position;

    // Baue finalen Clipspace-Vektor
    gl_Position = projection * view * vec4(worldPos, 0.0, 1.0);

    vUV = aUV;
}