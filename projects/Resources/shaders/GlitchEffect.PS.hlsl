#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer Material : register(b1)
{
    float time;
};

// 疑似ランダム関数
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

    // --------------------------------
    // linenoise（Y軸でラインごとのシフト）
    float lineNoise = rand(float2(floor(uv.y * 100.0 + time * 60.0), floor(uv.x * 30.0)));
    if (lineNoise < 0.5)
    {
    // 大きめのずれ & ランダムに強弱をつける
        float shiftSeed = rand(float2(uv.y * 100.0, time * 40.0));
        float shiftAmount = lerp(0.1, 0.4, shiftSeed); // 0.1〜0.4 の間で変化
        float shift = (rand(float2(time * 80.0, uv.y * 100.0)) - 0.5) * shiftAmount;
        uv.x += shift;
    }

    // RGBチャンネルずらし
    float chromaRand = rand(uv * time * 100.0);
    float2 chromaOffset = float2(chromaRand * 0.01, 0.0);

    float r = gTexture.Sample(gSampler, uv + chromaOffset).r;
    float g = gTexture.Sample(gSampler, uv).g;
    float b = gTexture.Sample(gSampler, uv - chromaOffset).b;

    float4 color = float4(r, g, b, 1.0f);

    // ブロックノイズ
    float blockNoise = rand(floor(uv * 30.0 + time * 10.0));
    if (blockNoise < 0.07)
    {
        color.rgb = float3(blockNoise, blockNoise * 0.8, blockNoise * 0.6);
    }

    // スキャンラインノイズ
    float scan = sin((uv.y + time * 2.0) * 300.0) * 0.02;
    color.rgb += scan;

    // 出力
    PixelShaderOutput output;
    output.color = color;
    return output;
}