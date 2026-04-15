#include "Particle.hlsli"

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    float4 color = input.color * textureColor;

    // ほぼ透明な外周を捨てる
    if (color.a < 0.08f)
    {
        discard;
    }

    output.color = color;
    return output;
}