#include "Object3d.hlsli"

struct ObjectParams
{
    bool useEnvironmentMap;
    float3 _padding;
    float4 color;
};

ConstantBuffer<ObjectParams> objectParam : register(b5);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

float rand2(float2 p)
{
    return frac(sin(dot(p, float2(12.9898f, 78.233f))) * 43758.5453f);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    float noise = rand2(input.texcoord * 80.0f + input.worldPosition.xy * 2.0f);
    float stripe = step(0.85f, frac(input.texcoord.y * 20.0f + input.worldPosition.x * 2.0f));

    float3 base = textureColor.rgb * objectParam.color.rgb;

    float3 errorColor = base;
    errorColor.r += 0.35f * noise;
    errorColor.g += 0.08f * noise;
    errorColor.b += 0.12f * (1.0f - noise);

    errorColor += stripe * float3(0.25f, 0.02f, 0.02f);

    output.color.rgb = saturate(errorColor);
    output.color.a = textureColor.a * objectParam.color.a;

    if (textureColor.a <= 0.5f)
    {
        discard;
    }

    if (output.color.a == 0.0f)
    {
        discard;
    }

    return output;
}