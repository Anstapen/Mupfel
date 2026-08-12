#include "quad.slang"

struct GeometryInstance {
    float2 pos1;
	float2 pos2;
	float4 color;
	uint   shape;
	float  _pad0;
	float2 inner_edge;
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
    float2 pos = input.inPos;
    float2 coord = input.inTexCoord;

    float2 offset = ExpandQuad(input.inPos, float2(t.pos2.x, t.pos2.y), 0.0);

    output.pos          = float4(t.pos1 + offset, 0.0, 1.0);
    
    // Save the original quad positions, but now scaled by a factor of 2
    // This trivialises the radius check for circles, as everything with a length > 1 can be discarded.
    output.shape_pos = input.inPos * 2.0;
    output.color = t.color;
    output.shape = t.shape;
    output.inner_edge = t.inner_edge;
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