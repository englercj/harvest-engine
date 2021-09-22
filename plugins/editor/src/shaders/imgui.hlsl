// Copyright Chad Engler

cbuffer vertexCB
{
    float4x4 ProjectionMatrix;
};

struct VS_INPUT
{
    float2 pos : POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

SamplerState sampler0;
Texture2D texture0;

// http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
//float3 SRGBToLinear(float3 value)
//{
//    return value * (value * (value * 0.305306011 + 0.682171111) + 0.012522878);
//}

//float SRGBToLinear(float value)
//{
//    if (value <= 0.04045)
//        return value / 12.92;
//    else
//        return pow((value + 0.055) / 1.055, 2.4);
//}
//
//float3 SRGBToLinear3(float3 value)
//{
//    return float3(SRGBToLinear(value.r), SRGBToLinear(value.g), SRGBToLinear(value.b));
//}

[shader("vertex")]
PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.col.rgb = input.col.rgb;
    output.col.a = input.col.a;
    output.uv  = input.uv;
    return output;
}

[shader("pixel")]
float4 ps_main(PS_INPUT input) : SV_Target
{
    float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
    return out_col;
}
