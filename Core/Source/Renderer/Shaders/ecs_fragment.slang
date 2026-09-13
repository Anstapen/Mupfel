struct VSOutput
{
    float4 pos : SV_Position;
    float2 fragTexCoord;
    nointerpolation uint textureIndex : TEXCOORD1;
    nointerpolation uint frame        : TEXCOORD2;
};

static const uint kMaxImages = 4096;

[[vk::binding(0, 0)]] Texture2DArray<float4> textures[kMaxImages];
[[vk::binding(1, 0)]] SamplerState texSampler;

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    return textures[NonUniformResourceIndex(vertIn.textureIndex)]
        .Sample(texSampler, float3(vertIn.fragTexCoord, float(vertIn.frame)));
}