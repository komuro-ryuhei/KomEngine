#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;

    float width, height;
    gTexture.GetDimensions(width, height);
    float2 texel = float2(1.0f / width, 1.0f / height);

    float4 baseColor = gTexture.Sample(gSampler, uv);

    // 明るい部分だけ抽出
    float brightness = dot(baseColor.rgb, float3(0.299f, 0.587f, 0.114f));
    float threshold = 0.65f;
    float bloomMask = saturate((brightness - threshold) * 3.0f);

    // 周囲をぼかして発光っぽくする
    float3 blur = float3(0.0f, 0.0f, 0.0f);

    blur += gTexture.Sample(gSampler, uv + texel * float2(-4, 0)).rgb;
    blur += gTexture.Sample(gSampler, uv + texel * float2(4, 0)).rgb;
    blur += gTexture.Sample(gSampler, uv + texel * float2(0, -4)).rgb;
    blur += gTexture.Sample(gSampler, uv + texel * float2(0, 4)).rgb;

    blur += gTexture.Sample(gSampler, uv + texel * float2(-3, -3)).rgb;
    blur += gTexture.Sample(gSampler, uv + texel * float2(3, -3)).rgb;
    blur += gTexture.Sample(gSampler, uv + texel * float2(-3, 3)).rgb;
    blur += gTexture.Sample(gSampler, uv + texel * float2(3, 3)).rgb;

    blur *= 1.0f / 8.0f;

    // 明るいところだけ発光を乗せる
    float bloomIntensity = 0.45f;
    float3 color = baseColor.rgb + blur * bloomMask * bloomIntensity;

    // 少しだけ青寄せして近未来感を足す
    color *= float3(0.92f, 1.03f, 1.12f);

    // 軽いビネット
    float2 center = uv - 0.5f;
    float vignette = 1.0f - dot(center, center) * 0.75f;
    color *= saturate(vignette);

    output.color = float4(saturate(color), baseColor.a);
    return output;
}