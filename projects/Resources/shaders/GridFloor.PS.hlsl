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
    int useEnvironmentMap;
    float3 padding;
    float4 color;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);
ConstantBuffer<SpotLight> gSpotLight : register(b4);
ConstantBuffer<ObjectParams> objectParam : register(b5);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
SamplerState gSampler : register(s0);

float GridLine(float2 p, float scale, float lineWidth)
{
    float2 coord = p * scale;
    float2 fw = max(fwidth(coord), float2(1.0e-4f, 1.0e-4f));
    float2 cell = abs(frac(coord) - 0.5f) / fw;
    float d = min(cell.x, cell.y);
    return 1.0f - saturate(d - lineWidth);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 p = input.worldPosition.xz;

    // 暗い床色
    float3 baseColor = float3(0.005f, 0.012f, 0.030f);

    // 細かいグリッド
    float minor = GridLine(p, 0.35f, 0.15f);

    // 少し太いグリッド
    float major = GridLine(p, 0.07f, 0.22f);

    // 中央軸を少しだけ強調
    float axisWidth = 0.035f;
    float axisX = 1.0f - smoothstep(0.0f, axisWidth, abs(p.x));
    float axisZ = 1.0f - smoothstep(0.0f, axisWidth, abs(p.y));
    float axis = max(axisX, axisZ);

    // カメラ距離でフェード
    float dist = length(input.worldPosition.xz - gCamera.worldPosition.xz);
    float fade = saturate(1.0f - dist / 120.0f);
    fade = 0.10f + 0.90f * pow(fade, 1.8f);

    float3 minorColor = float3(0.12f, 0.36f, 0.95f);
    float3 majorColor = float3(0.20f, 0.58f, 1.00f);
    float3 axisColor = float3(0.45f, 0.82f, 1.00f);

    float3 color = baseColor;
    color += minorColor * minor * 0.28f * fade;
    color += majorColor * major * 0.65f * fade;
    color += axisColor * axis * 0.55f * fade;

    // 少しだけ地平線方向の青味を足す
    color += float3(0.00f, 0.02f, 0.05f) * fade * 0.25f;

    // Object3d 側の色乗算は活かす
    color *= objectParam.color.rgb;

    output.color = float4(saturate(color), objectParam.color.a);
    return output;
}