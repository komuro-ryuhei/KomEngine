#include "Skybox.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

TextureCube<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float3 dir = normalize(input.texcoord);
    float3 tex = gTexture.Sample(gSampler, dir).rgb;

    // y: [-1, +1] -> [0, 1]
    float y01 = saturate(dir.y * 0.5f + 0.5f);

    // かなり黒寄りの配色
    float3 topColor = float3(0.0006f, 0.0008f, 0.0030f);
    float3 midColor = float3(0.0020f, 0.0040f, 0.0120f);
    float3 bottomColor = float3(0.0012f, 0.0025f, 0.0070f);

    float3 baseColor;
    if (y01 >= 0.5f)
    {
        float t = (y01 - 0.5f) / 0.5f;
        baseColor = lerp(midColor, topColor, t);
    }
    else
    {
        float t = y01 / 0.5f;
        baseColor = lerp(bottomColor, midColor, t);
    }

    // 地平線の青い帯をかなり細く、弱くする
    float horizonLine = exp(-abs(dir.y) * 52.0f);
    baseColor += float3(0.006f, 0.018f, 0.060f) * horizonLine * 0.42f;

    // 下側の持ち上げもかなり弱く
    float lowerMask = pow(saturate(1.0f - y01), 3.5f);
    baseColor += float3(0.000f, 0.0015f, 0.005f) * lowerMask * 0.10f;

    // DDSはほぼ無視
    baseColor += tex * 0.003f;

    // 既存の SetColor() は活かす
    baseColor *= gMaterial.color.rgb;

    output.color = float4(saturate(baseColor), 1.0f);
    return output;
}