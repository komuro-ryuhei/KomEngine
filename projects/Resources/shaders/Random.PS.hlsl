#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer MaterialBuffer : register(b1)
{
    float time;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float rand2dTo1d(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233f))) * 43758.5453f);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // 
    float2 noiseUV = input.texcoord * 10.0f;

    // 時間でオフセット
    noiseUV += float2(time * 1.0f, time * 0.5f);

    // ランダムノイズ生成
    float random = rand2dTo1d(noiseUV);

    // グレースケール出力
    output.color = float4(random, random, random, 1.0f);

    return output;
}