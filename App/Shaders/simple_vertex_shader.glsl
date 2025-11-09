#version 460 core
#extension GL_ARB_gpu_shader_int64 : require

layout (location = 0) in vec3 aPos;    // Quad-Vertex (statisch)
layout (location = 1) in vec2 aUV;     // Quad-UV (statisch)

struct TransformData {
    vec2 pos;
    vec2 scale;
    vec2 rotation; // y component of rotation is padding on cpu side! dont use!
};

struct TextureData {
    uint64_t tex_id;
    uint64_t pad1;
    uint64_t pad2;
};

layout(std430, binding = 3) readonly buffer TransformComponents {
    TransformData transforms[];
};

layout(std430, binding = 6) readonly buffer TextureComponents {
    TextureData textures[];
};

struct ActiveEntity {
    uint e;   // Entity ID
    uint ti;  // Transform dense index
    uint tei;  // Texture dense index
};

layout(std430, binding = 7) buffer ActiveEntities {
    ActiveEntity pairs[];
};

uniform mat4 view;
uniform mat4 projection;

out vec2 vUV;

void main()
{
    uint instanceID = gl_InstanceID;

    if(pairs[instanceID].e == 0) return;

    uint transform_index = pairs[instanceID].ti;
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