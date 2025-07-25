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

    // 中心からの距離
    float2 center = float2(0.5, 0.5);
    float2 delta = uv - center;
    float dist = length(delta);

    // 時間によって動く波の位置
    float wave = sin((dist - time * 0.8) * 80.0) * 0.01;

    // 効果範囲（だんだん消える）
    float width = 0.02;
    float fade = smoothstep(0.0, 1.0, 1.0 - abs(dist - time * 0.8) / width);

    // 歪ませるUV
    float2 distortedUV = uv + normalize(delta) * wave * fade;

    // テクスチャサンプリング
    float4 col = gTexture.Sample(gSampler, distortedUV);

    PixelShaderOutput output;
    output.color = col;
    return output;
}