#include "quad.slang"

struct GeometryInstance {
    float2 pos1;
    float2 axis_u;
    float4 color;
    float2 axis_v;
    float2 inner_edge;
    uint   shape;
    float  _pad0;
    float2 _pad1;
};

[[vk::binding(0, 0)]]
StructuredBuffer<GeometryInstance> geo_instances;

struct VSOutput
{
    float4 pos : SV_Position;
    nointerpolation float4 color;
    float2 shape_pos;
    nointerpolation uint shape;
    nointerpolation float2 inner_edge;
};

[shader("vertex")]
VSOutput vertMain(QuadVSInput input) {
    VSOutput output;
    GeometryInstance t = geo_instances[input.instanceIndex];

    float2 offset = input.inPos.x * t.axis_u + input.inPos.y * t.axis_v;

    output.pos        = float4(t.pos1 + offset, 0.0, 1.0);
    output.shape_pos  = input.inPos * 2.0;
    output.color      = t.color;
    output.inner_edge = t.inner_edge;
    output.shape      = t.shape;
    return output;
}