#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer MaterialBuffer : register(b1)
{
    float gTime;
    float3 padding0;

    // SetPostEffectParam(param0, param1, param2, param3)
    float gIntensity; // param0 集中線の濃さ
    float gLineCount; // param1 線の数
    float gLineWidth; // param2 線の太さ
    float gSpeed; // param3 流れる速さ
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float Random01(float x)
{
    return frac(sin(x * 12.9898f) * 43758.5453f);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;
    float4 sceneColor = gTexture.Sample(gSampler, uv);

    // 画面中央基準
    float2 center = float2(0.5f, 0.5f);
    float2 p = uv - center;

    // 画面比率補正
    p.x *= 1280.0f / 720.0f;

    float dist = length(p);

    // 中心付近は空ける
    float innerMask = smoothstep(0.12f, 0.22f, dist);

    // 外側ほど強く
    float outerMask = smoothstep(0.20f, 0.95f, dist);

    // 画面端で少し消す
    float screenEdgeMask = 1.0f - smoothstep(1.05f, 1.35f, dist);

    // 角度を 0〜1 に変換
    float angle = atan2(p.y, p.x);
    float angle01 = (angle + 3.14159265f) / 6.2831853f;

    float count = max(gLineCount, 1.0f);
    float sector = angle01 * count;

    float sectorId = floor(sector);
    float sectorLocal = frac(sector);

    // 線ごとのランダム
    float rnd0 = Random01(sectorId + 1.0f);
    float rnd1 = Random01(sectorId + 5.0f);

    // ランダムに少し角度をずらす
    float shiftedLocal = frac(sectorLocal + rnd0 * 0.35f);

    // セクター中央に細い線を作る
    float centerDistance = abs(shiftedLocal - 0.5f);

    float width = max(gLineWidth, 0.001f);
    float speedLineMask = 1.0f - smoothstep(width, width + 0.015f, centerDistance);

    // 線を間引く
    speedLineMask *= step(0.25f, rnd1);

    // 距離方向に流れるマスク
    float flow = frac(dist * 4.0f - gTime * gSpeed + rnd0);
    float flowMask = 1.0f - smoothstep(0.20f, 0.70f, flow);

    float mask = speedLineMask;
    mask *= flowMask;
    mask *= innerMask;
    mask *= outerMask;
    mask *= screenEdgeMask;
    mask *= saturate(gIntensity);

    mask = saturate(mask);

    // 白い集中線
    float3 speedLineColor = float3(1.0f, 1.0f, 1.0f);

    output.color.rgb = lerp(sceneColor.rgb, speedLineColor, mask);
    output.color.a = sceneColor.a;

    return output;
}