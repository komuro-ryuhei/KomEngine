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

    // 元の色を取得
    float4 baseColor = gTexture.Sample(gSampler, uv);

    // 色を反転（R/G/Bだけ。Aはそのまま）
    float3 invertedColor = 1.0 - baseColor.rgb;

    PixelShaderOutput output;
    output.color = float4(invertedColor, baseColor.a);
    return output;
}