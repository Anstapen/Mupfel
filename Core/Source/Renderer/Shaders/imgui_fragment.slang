struct VSOutput {
    float4 pos : SV_Position;
    float4 color;
    float2 uv;
};

[[vk::binding(0, 0)]]
Texture2D fontTexture;

[[vk::binding(1, 0)]]
SamplerState fontSampler;

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    return vertIn.color * fontTexture.Sample(fontSampler, vertIn.uv);
}