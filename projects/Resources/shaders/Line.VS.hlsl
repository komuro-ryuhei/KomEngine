#include "Line.hlsli"

cbuffer Camera : register(b0)
{
    float4x4 viewProj;
};

struct LineVertexInput
{
    float3 position : POSITION0;
    float4 color : COLOR0;
};

LineVertexOutput main(LineVertexInput input)
{
    LineVertexOutput output;

    float4 pos = float4(input.position, 1.0f);
    output.position = mul(pos, viewProj);
    output.color = input.color;

    return output;
}