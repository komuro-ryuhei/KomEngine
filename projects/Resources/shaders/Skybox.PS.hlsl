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

    // キューブマップ方向
    float3 dir = normalize(input.texcoord);

    // 今回は真っ黒DDSを土台にして、色はシェーダーで作る
    // 念のため少しだけ読んでおくが、ほぼ使わない
    float3 tex = gTexture.Sample(gSampler, dir).rgb;

    // y: [-1, +1] -> [0, 1]
    float y01 = saturate(dir.y * 0.5f + 0.5f);

    // 色設計
    float3 topColor = float3(0.005f, 0.008f, 0.020f); // 上はかなり暗い
    float3 horizonColor = float3(0.050f, 0.110f, 0.280f);
    float3 bottomColor = float3(0.015f, 0.030f, 0.080f);

    float3 baseColor;

    // 上半分 / 下半分でグラデーション
    if (y01 >= 0.5f)
    {
        float t = (y01 - 0.5f) / 0.5f; // 0..1
        baseColor = lerp(horizonColor, topColor, t);
    }
    else
    {
        float t = y01 / 0.5f; // 0..1
        baseColor = lerp(bottomColor, horizonColor, t);
    }

    // 地平線の発光帯
    // dir.y = 0 付近を強くする
    float horizonBand = exp(-abs(dir.y) * 18.0f);
    float3 glowColor = float3(0.030f, 0.090f, 0.220f);
    baseColor += glowColor * horizonBand * 0.8f;

    // 下側をほんの少し持ち上げる
    float lowerLift = pow(saturate(1.0f - y01), 2.0f);
    baseColor += float3(0.000f, 0.010f, 0.030f) * lowerLift * 0.35f;

    // テクスチャはほぼ無視。真っ黒DDSでも問題ない
    baseColor += tex * 0.02f;

    // 既存の SetColor() を活かす
    // 通常は (1,1,1,1)、怒り時の赤味変更にも使える
    baseColor *= gMaterial.color.rgb;

    output.color = float4(saturate(baseColor), 1.0f);
    return output;
}