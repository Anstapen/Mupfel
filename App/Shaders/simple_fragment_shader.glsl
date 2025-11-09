#version 430 core
#extension GL_ARB_bindless_texture : require

in vec2 vUV;
//in vec4 vColor;
//in float vTexIndex;

out vec4 FragColor;

// Variante A: eine einzige Textur (einfach)
uniform sampler2D uTex;

// Variante B (optional): Texture-Array (besser für viele Texturen)
//uniform sampler2DArray uTexArray;

layout(std430, binding = 12) readonly buffer TextureHandles {
    sampler2D textures;
};

void main()
{
    // A) Einzel-Textur:
    FragColor = texture(textures, vUV);

    // B) Texture-Array: Schicht durch vTexIndex wählen
    //FragColor = texture(uTexArray, vec3(vUV, vTexIndex));

    //FragColor = texCol * vColor;
}