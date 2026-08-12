#include "quad.slang"

static const uint SHAPE_NONE   = 0;
static const uint SHAPE_RECT   = 1;
static const uint SHAPE_LINE   = 2;
static const uint SHAPE_CIRCLE = 3;

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

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {

    if(vertIn.color.w < 0.1f)
    {
        discard;
    }

    uint shape = vertIn.shape;

    if(shape == 1) // Rectangle
    {
        // Discard all pixels 
        if (all(abs(vertIn.shape_pos) < vertIn.inner_edge))
        {
            discard;
        }

        return vertIn.color;
    }

    if(shape == 2) // Line
    {
        return vertIn.color;
    }

    if(shape == 3) // Circle
    {
        float sqrt_length = dot(vertIn.shape_pos, vertIn.shape_pos);

        // Discard pixels that lay outside the circle
        if (sqrt_length > 1.0)
        {
            discard;
        }
        float inner = vertIn.inner_edge.x;
        if (sqrt_length < inner * inner)
        {
            discard;
        }
        return vertIn.color;
    }

    

    return vertIn.color;
}