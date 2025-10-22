#version 330 core

in vec2 vUV;
//in vec4 vColor;
//in float vTexIndex;

out vec4 FragColor;

// Variante A: eine einzige Textur (einfach)
uniform sampler2D uTex;

// Variante B (optional): Texture-Array (besser für viele Texturen)
//uniform sampler2DArray uTexArray;

void main()
{
    // A) Einzel-Textur:
    FragColor = texture(uTex, vUV);

    // B) Texture-Array: Schicht durch vTexIndex wählen
    //FragColor = texture(uTexArray, vec3(vUV, vTexIndex));

    //FragColor = texCol * vColor;
}