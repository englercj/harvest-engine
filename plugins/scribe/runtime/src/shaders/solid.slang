// Copyright Chad Engler
//
// Companion shader for simple retained text quads such as underline and strike-through.

struct SolidVertexInput
{
    float2 pos : ATTRIB0;
    float4 col : ATTRIB1;
};

struct SolidVertexOutput
{
    float4 position : SV_Position;
    float4 color : U_COLOR;
};

cbuffer ParamStruct : register(b0)
{
    float4 mvp[4];
    float4 viewport;
};

[shader("vertex")]
SolidVertexOutput solid_vs_main(SolidVertexInput input)
{
    SolidVertexOutput output;
    output.position.x = input.pos.x * mvp[0].x + input.pos.y * mvp[0].y + mvp[0].w;
    output.position.y = input.pos.x * mvp[1].x + input.pos.y * mvp[1].y + mvp[1].w;
    output.position.z = input.pos.x * mvp[2].x + input.pos.y * mvp[2].y + mvp[2].w;
    output.position.w = input.pos.x * mvp[3].x + input.pos.y * mvp[3].y + mvp[3].w;
    output.color = input.col;
    return output;
}

[shader("pixel")]
float4 solid_ps_main(SolidVertexOutput input) : SV_Target
{
    return input.color;
}
