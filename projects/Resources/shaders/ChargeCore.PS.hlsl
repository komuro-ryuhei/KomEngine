#include "Object3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
};

struct SpotLight
{
    float4 color;
    float3 position;
    float intensity;
    float3 direction;
    float distance;
    float decay;
    float casAngle;
    float cosFalloffStart;
};

struct Camera
{
    float3 worldPosition;
};

struct ObjectParams
{
    bool useEnvironmentMap;
    float3 _padding;
    float4 color;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);
ConstantBuffer<SpotLight> gSpotLight : register(b4);
ConstantBuffer<ObjectParams> objectParam : register(b5);

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);

SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float Hash21(float2 p)
{
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return frac(p.x * p.y);
}

float Noise2D(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    float a = Hash21(i);
    float b = Hash21(i + float2(1.0, 0.0));
    float c = Hash21(i + float2(0.0, 1.0));
    float d = Hash21(i + float2(1.0, 1.0));

    float2 u = f * f * (3.0 - 2.0 * f);

    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float2 uv = transformedUV.xy;

    float4 textureColor = gTexture.Sample(gSampler, uv);

    // カメラ方向
    float3 N = normalize(input.normal);
    float3 V = normalize(gCamera.worldPosition - input.worldPosition);

    // フレネルで外周発光
    float fresnel = pow(1.0f - saturate(dot(N, V)), 3.5f);

    // UV中心からの距離
    float2 centered = uv * 2.0f - 1.0f;
    float dist = length(centered);

    // 中心が強いグロー
    float core = saturate(1.0f - dist);
    core = pow(core, 2.8f);

    // 外周寄りのオーラ
    float aura = saturate(1.0f - abs(dist - 0.45f) * 2.2f);
    aura = pow(aura, 1.8f);

    // 回転しているモデルのワールド座標に依存するノイズ
    float n1 = Noise2D(input.worldPosition.xy * 2.0f + uv * 4.0f);
    float n2 = Noise2D(input.worldPosition.zy * 3.2f - uv * 3.0f);
    float noise = (n1 * 0.6f + n2 * 0.4f);

    // 表面のムラ
    float energy = core + aura * 0.45f + fresnel * 0.85f + noise * 0.18f;
    energy = saturate(energy);

    // 色
    float3 innerColor = float3(1.0f, 1.0f, 1.0f);
    float3 midColor = float3(0.45f, 0.85f, 1.0f);
    float3 rimColor = float3(0.12f, 0.55f, 1.0f);

    float3 colorA = lerp(midColor, innerColor, core);
    float3 colorB = lerp(colorA, rimColor, fresnel * 0.65f);

    // 既存のC++側 color も活かす
    float3 finalRgb = colorB * gMaterial.color.rgb * objectParam.color.rgb;

    // emissiveっぽく強める
    finalRgb *= (1.2f + fresnel * 1.0f + core * 0.8f);

    // α
    float alpha = textureColor.a * gMaterial.color.a * objectParam.color.a;
    alpha *= saturate(core * 0.85f + fresnel * 0.8f + aura * 0.35f);
    alpha = max(alpha, fresnel * 0.25f);

    output.color = float4(finalRgb * energy, alpha);

    if (output.color.a <= 0.01f)
    {
        discard;
    }

    return output;
}