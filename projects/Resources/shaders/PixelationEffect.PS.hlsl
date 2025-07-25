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

    // Pixelation のサイズ（値を大きくすると荒くなる）
    float pixelSize = 0.02;

    // UVを丸めて、サンプリング座標を固定化
    float2 blockUV = floor(uv / pixelSize) * pixelSize;

    float4 color = gTexture.Sample(gSampler, blockUV);

    PixelShaderOutput output;
    output.color = color;
    return output;
}