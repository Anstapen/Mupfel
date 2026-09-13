struct VSInput {
    float2 inPos;
    float2 inTexCoord;
};

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
[[vk::binding(0, 2)]]
StructuredBuffer<TextureInstance> texture_instances;

struct VSOutput
{
    float4 pos : SV_Position;
    float2 fragTexCoord;
    nointerpolation uint textureIndex : TEXCOORD1;
    nointerpolation uint frame        : TEXCOORD2;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.pos = mul(ubo.proj, mul(ubo.view, float4(input.inPos, 0.0, 1.0)));
    output.fragTexCoord = input.inTexCoord;
    output.textureIndex = 1;
    output.frame = 0;
    return output;
}