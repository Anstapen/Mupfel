#version 430 core
#extension GL_ARB_bindless_texture : require

in vec2 vUV;

out vec4 FragColor;

layout(std430, binding = 12) readonly buffer TextureHandles {
    sampler2D textures;
};

void main()
{
    FragColor = texture(textures, vUV);
}