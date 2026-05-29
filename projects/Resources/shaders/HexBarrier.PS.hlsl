#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer MaterialBuffer : register(b1)
{
    float gTime;
    float3 padding0;

    float gProgress; // param0
    float gAlpha; // param1
    float gHexScale; // param2
    float gLineWidth; // param3
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float HexGridLine(float2 uv, float scale, float lineWidth)
{
    // 画面比率補正
    uv.x *= 1280.0f / 720.0f;

    // 中心基準
    float2 p = uv * 2.0f - 1.0f;
    p *= scale;

    const float SQRT3 = 1.7320508f;

    // 3方向のラインを重ねて六角形風にする
    float l1 = abs(frac(p.x) - 0.5f);
    float l2 = abs(frac(p.x * 0.5f + p.y * SQRT3 * 0.5f) - 0.5f);
    float l3 = abs(frac(p.x * 0.5f - p.y * SQRT3 * 0.5f) - 0.5f);

    float d = min(l1, min(l2, l3));

    return 1.0f - smoothstep(lineWidth, lineWidth + 0.01f, d);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;
    float4 sceneColor = gTexture.Sample(gSampler, uv);

    float2 center = float2(0.5f, 0.5f);
    float2 diff = uv - center;
    diff.x *= 1280.0f / 720.0f;

    float dist = length(diff);

    float progress = saturate(gProgress);
    float alphaParam = saturate(gAlpha);

    // 画面中央から外へ広がる半径
    float radius = lerp(0.02f, 0.95f, progress);

    // 半径内だけ表示
    float insideMask = 1.0f - smoothstep(radius, radius + 0.035f, dist);

    // 外周の強い光
    float edgeGlow = 1.0f - smoothstep(0.0f, 0.045f, abs(dist - radius));

    // 六角形ライン
    float hexLine = HexGridLine(uv, gHexScale, gLineWidth);

    // 球体っぽく端を少し薄くする
    float sphereFade = 1.0f - smoothstep(0.65f, 0.95f, dist);

    // 中心が濃くなりすぎないようにする
    float centerFade = smoothstep(0.03f, 0.22f, dist);

    float barrier = hexLine * insideMask * sphereFade * centerFade;
    barrier += edgeGlow * 0.85f;

    // 少し発光の揺れ
    float flicker = 0.85f + 0.15f * sin(gTime * 35.0f);
    barrier *= flicker;

    float3 barrierColor = float3(0.35f, 0.95f, 1.0f);

    float finalAlpha = saturate(barrier * alphaParam);

    output.color.rgb = sceneColor.rgb + barrierColor * finalAlpha;
    output.color.a = sceneColor.a;

    return output;
}