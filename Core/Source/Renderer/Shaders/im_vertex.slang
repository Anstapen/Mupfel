#include "quad.slang"

struct TextureInstance {
    float2 pos;
    float width;
    float height;
    float  rotation;
    uint   textureIndex;
    float  uvScale;
	float  _pad1;
};

[[vk::binding(2, 0)]]
StructuredBuffer<TextureInstance> texture_instances;

struct VSOutput
{
    float4 pos : SV_Position;
    float2 fragTexCoord;
    nointerpolation uint textureIndex : TEXCOORD1;
};

[shader("vertex")]
VSOutput vertMain(QuadVSInput input) {
    VSOutput output;
    TextureInstance t = texture_instances[input.instanceIndex];
    float2 offset = ExpandQuad(input.inPos, float2(t.width, t.height), t.rotation);

    output.pos          = float4(t.pos + offset, 0.0, 1.0);
    output.fragTexCoord = input.inTexCoord * t.uvScale;
    output.textureIndex = t.textureIndex;
    return output;
}