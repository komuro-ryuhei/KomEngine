#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer Material : register(b1)
{
    float time;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    float2 uv = input.texcoord;

    // ずらす量
    float offsetStrength = 0.015f;

    // 中心からの方向ベクトル
    float2 center = float2(0.5, 0.5);
    float2 dir = normalize(uv - center);

    // チャンネルごとのオフセット
    float2 rOffset = dir * offsetStrength;
    float2 bOffset = -dir * offsetStrength;

    float r = gTexture.Sample(gSampler, uv + rOffset).r;
    float g = gTexture.Sample(gSampler, uv).g;
    float b = gTexture.Sample(gSampler, uv + bOffset).b;

    float4 color = float4(r, g, b, 1.0f);

    PixelShaderOutput output;
    output.color = color;
    return output;
}