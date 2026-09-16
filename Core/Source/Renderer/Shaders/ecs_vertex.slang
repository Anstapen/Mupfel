#include "quad.slang"

struct UniformBuffer {
    float4x4 view;
    float4x4 proj;
    float4   cameraPos;
};
[[vk::binding(2, 0)]]
ConstantBuffer<UniformBuffer> ubo;

struct TextureInstance {
    float3 pos;
    float  _pad0;
    float2 scale;
    float  rotation;
    uint   textureIndex;
    float  _pad1;
    uint   emitsLight;
    uint   frame;
    float  _pad2;
};
[[vk::binding(3, 0)]]
StructuredBuffer<TextureInstance> texture_instances;

struct VSOutput
{
    float4 pos : SV_Position;
    float2 fragTexCoord;
    nointerpolation uint textureIndex : TEXCOORD1;
    nointerpolation uint frame        : TEXCOORD2;
    nointerpolation uint emitsLight;
    float3 worldPos;
    float3 worldNormal;
};

[shader("vertex")]
VSOutput vertMain(QuadVSInput input) {
    VSOutput output;

    TextureInstance t = texture_instances[input.instanceIndex];

    float2 offset   = ExpandQuad(input.inPos, t.scale, t.rotation);
    float3 worldPos = t.pos + float3(offset, 0.0);
    output.worldPos = worldPos;
    output.worldNormal = float3(0.0, 0.0, 1.0);
    output.pos          = mul(ubo.proj, mul(ubo.view, float4(worldPos, 1.0)));
    output.fragTexCoord = input.inTexCoord;
    output.textureIndex = t.textureIndex;
    output.frame = t.frame;
    output.emitsLight = t.emitsLight;
    return output;
}