static const uint kMaxImages = 4096;

[[vk::binding(0, 0)]] Texture2DArray<float4> textures[kMaxImages];
[[vk::binding(1, 0)]] SamplerState texSampler;

struct VSOutput
{
    float4 pos : SV_Position;
    float2 fragTexCoord;
    nointerpolation uint textureIndex : TEXCOORD1;
};

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {

    float4 tex_color = textures[NonUniformResourceIndex(vertIn.textureIndex)].Sample(texSampler, float3(vertIn.fragTexCoord, 0));

    if(tex_color.w < 0.1f)
    {
        discard;
    }

    return tex_color;
}