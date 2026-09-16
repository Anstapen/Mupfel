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

struct UniformBuffer {
    float4x4 view;
    float4x4 proj;
    float4   cameraPos;
};


struct LightParams
{
    uint numLights;
};

struct LightInstance {
    float3 color;
    float  ambientLight;
    float3 pos;
    float _pad1;
};


static const uint kMaxImages = 4096;

[[vk::binding(0, 0)]] Texture2DArray<float4> textures[kMaxImages];
[[vk::binding(1, 0)]] SamplerState texSampler;
[[vk::binding(2, 0)]]
ConstantBuffer<UniformBuffer> ubo;
[[vk::push_constant]]
ConstantBuffer<LightParams> lightParams;
[[vk::binding(5, 0)]]
StructuredBuffer<LightInstance> light_instances;

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    float4 tex_color = textures[NonUniformResourceIndex(vertIn.textureIndex)].Sample(texSampler, float3(vertIn.fragTexCoord, float(vertIn.frame)));

    if(tex_color.w < 0.1f)
    {
        discard;
    }
   
    if((vertIn.emitsLight == 0) && (lightParams.numLights > 0))
    {
        var ambient = float3(0.0f);
        var diffuse = float3(0.0f);
        var specular = float3(0.0f);
        for(uint i = 0; i < lightParams.numLights; i++)
        {
            ambient  += light_instances[i].color * light_instances[i].ambientLight;

            float3 lightDir = normalize(light_instances[i].pos - vertIn.worldPos);
            float diff = max(dot(vertIn.worldNormal, lightDir), 0.0);
            diffuse += light_instances[i].color * diff;

            float3 viewDir = normalize(ubo.cameraPos.xyz - vertIn.worldPos);
            float3 reflectDir = reflect(-lightDir, vertIn.worldNormal);

            specular += pow(max(dot(viewDir, reflectDir), 0.0), 32);
        }

        tex_color.xyz = (ambient + diffuse + specular) * tex_color.xyz;

        return float4(tex_color.rgb, tex_color.a);
    }

    return tex_color;
}