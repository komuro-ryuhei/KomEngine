Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer Material : register(b0)
{
    float4 gColor;
    int gEnableLighting;
    float4x4 gUvTransform;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

float sdCircle(float2 p, float r)
{
    return length(p) - r;
}

float sdSegment(float2 p, float2 a, float2 b)
{
    float2 pa = p - a;
    float2 ba = b - a;
    float h = saturate(dot(pa, ba) / dot(ba, ba));
    return length(pa - ba * h);
}

float ringMask(float2 p, float r, float thickness)
{
    float d = abs(length(p) - r);
    return 1.0 - smoothstep(thickness, thickness + 0.003, d);
}

float lineMask(float2 p, float2 a, float2 b, float thickness)
{
    float d = sdSegment(p, a, b);
    return 1.0 - smoothstep(thickness, thickness + 0.003, d);
}

float bulletHole(float2 p, float2 c, float r)
{
    float d = length(p - c);
    return 1.0 - smoothstep(r, r + 0.01, d);
}

float crackLine(float2 p, float2 a, float2 b, float thickness)
{
    float d = sdSegment(p, a, b);
    return 1.0 - smoothstep(thickness, thickness + 0.002, d);
}

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.texcoord;
    float2 p = uv * 2.0 - 1.0;

    // 横長補正
    p.x *= 1280.0 / 720.0;

    float4 outColor = float4(0, 0, 0, 0);

    // -----------------------------
    // 薄いターゲットマーク
    // -----------------------------
    float ring1 = ringMask(p, 0.34, 0.006);
    float ring2 = ringMask(p, 0.20, 0.004);

    float crossV = lineMask(p, float2(0.0, -0.48), float2(0.0, 0.48), 0.0035);
    float crossH = lineMask(p, float2(-0.48, 0.0), float2(0.48, 0.0), 0.0035);

    float tickTop = lineMask(p, float2(0.0, 0.42), float2(0.0, 0.54), 0.006);
    float tickBottom = lineMask(p, float2(0.0, -0.42), float2(0.0, -0.54), 0.006);
    float tickLeft = lineMask(p, float2(-0.66, 0.0), float2(-0.50, 0.0), 0.006);
    float tickRight = lineMask(p, float2(0.50, 0.0), float2(0.66, 0.0), 0.006);

    float targetMask = max(max(ring1, ring2), max(max(crossV, crossH), max(max(tickTop, tickBottom), max(tickLeft, tickRight))));
    float3 targetCol = float3(0.90, 0.95, 1.00) * 0.22;

    outColor.rgb += targetCol * targetMask;
    outColor.a = max(outColor.a, targetMask * 0.22);

    // -----------------------------
    // X マーク
    // -----------------------------
    float x1 = lineMask(p, float2(-0.30, -0.30), float2(0.30, 0.30), 0.010);
    float x2 = lineMask(p, float2(-0.30, 0.30), float2(0.30, -0.30), 0.010);
    float xMask = max(x1, x2);

    float3 xCol = float3(1.0, 1.0, 1.0) * 0.72;
    outColor.rgb = lerp(outColor.rgb, xCol, xMask);
    outColor.a = max(outColor.a, xMask * 0.72);

    // -----------------------------
    // 弾痕
    // -----------------------------
    float hole1 = bulletHole(p, float2(-0.12, 0.10), 0.045);
    float hole2 = bulletHole(p, float2(0.16, -0.14), 0.040);
    float hole3 = bulletHole(p, float2(0.02, 0.02), 0.032);

    float holes = max(hole1, max(hole2, hole3));

    // 黒い穴
    outColor.rgb = lerp(outColor.rgb, float3(0.02, 0.02, 0.02), holes);
    outColor.a = max(outColor.a, holes * 0.95);

    // 焦げ縁
    float burn1 = ringMask(p - float2(-0.12, 0.10), 0.055, 0.010);
    float burn2 = ringMask(p - float2(0.16, -0.14), 0.050, 0.010);
    float burn3 = ringMask(p - float2(0.02, 0.02), 0.040, 0.008);
    float burn = max(burn1, max(burn2, burn3));

    outColor.rgb = lerp(outColor.rgb, float3(0.18, 0.16, 0.16), burn * 0.8);
    outColor.a = max(outColor.a, burn * 0.65);

    // -----------------------------
    // 軽いひび
    // -----------------------------
    float c1 = crackLine(p, float2(-0.12, 0.10), float2(-0.22, 0.18), 0.0025);
    float c2 = crackLine(p, float2(-0.12, 0.10), float2(-0.20, 0.02), 0.0020);
    float c3 = crackLine(p, float2(0.16, -0.14), float2(0.28, -0.20), 0.0025);
    float c4 = crackLine(p, float2(0.16, -0.14), float2(0.24, -0.04), 0.0020);

    float crack = max(max(c1, c2), max(c3, c4));
    outColor.rgb = lerp(outColor.rgb, float3(0.70, 0.70, 0.72), crack * 0.35);
    outColor.a = max(outColor.a, crack * 0.35);

    // 全体カラー係数
    outColor *= gColor;

    return outColor;
}