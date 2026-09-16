
static const uint SHAPE_NONE   = 0;
static const uint SHAPE_RECT   = 1;
static const uint SHAPE_LINE   = 2;
static const uint SHAPE_CIRCLE = 3;

struct VSOutput
{
    float4 pos : SV_Position;
    nointerpolation float4 color;
    float2 shape_pos;
    nointerpolation uint shape;
    nointerpolation float2 inner_edge;
};

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {

    if(vertIn.color.w < 0.1f)
    {
        discard;
    }

    uint shape = vertIn.shape;

    if(shape == SHAPE_RECT)
    {
        // Discard all pixels 
        if (all(abs(vertIn.shape_pos) < vertIn.inner_edge))
        {
            discard;
        }

        return vertIn.color;
    }

    if(shape == SHAPE_LINE)
    {
        return vertIn.color;
    }

    if(shape == SHAPE_CIRCLE)
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