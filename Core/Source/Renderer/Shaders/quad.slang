struct QuadVSInput
{
    float2 inPos;
    float2 inTexCoord;
    uint   instanceIndex : SV_InstanceID;
};

/** Expand and rotate the unit quad. */
float2 ExpandQuad(float2 inPos, float2 size, float rotation)
{
    float2 p = inPos * size;
    float  s = sin(rotation);
    float  c = cos(rotation);
    return float2(p.x * c - p.y * s, p.x * s + p.y * c);
}