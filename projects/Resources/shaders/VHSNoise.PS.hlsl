#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer Material : register(b1)
{
    float time;
};

float rand(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    float2 uv = input.texcoord;
    float2 originalUV = uv;

    // -----------------------------
    // Scanline（スキャンライン効果）
    float scan = sin((uv.y + time * 2.0) * 600.0) * 0.04;
    uv.x += scan;

    // -----------------------------
    // 横ノイズ（ゆらゆら揺れる感じ）
    uv.x += sin((uv.y + time * 3.0) * 80.0) * 0.005;

    // -----------------------------
    // ランダムな横ズレ（時々強く）
    float lineJitter = rand(float2(floor(uv.y * 100.0), floor(time * 20.0)));
    if (lineJitter < 0.05)
    {
        uv.x += 0.1 * rand(float2(uv.y, time));
    }

    // -----------------------------
    // RGBずれ（クロマティックアバレーション）
    float2 offset = float2(0.003, 0.0);
    float r = gTexture.Sample(gSampler, uv + offset).r;
    float g = gTexture.Sample(gSampler, uv).g;
    float b = gTexture.Sample(gSampler, uv - offset).b;

    float4 color = float4(r, g, b, 1.0f);

    // -----------------------------
    // 点滅（フリッカー）
    float flicker = sin(time * 50.0 + uv.y * 300.0);
    color.rgb += flicker * 0.03;

    // -----------------------------
    // ブロックノイズ
    float block = rand(floor(uv * 20.0 + time * 3.0));
    if (block < 0.02)
    {
        color.rgb = float3(block, block, block);
    }

    PixelShaderOutput output;
    output.color = color;
    return output;
}