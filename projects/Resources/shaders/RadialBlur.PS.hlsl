#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    const float2 kCenter = float2(0.5, 0.5); // 中心点
    const uint kNumSamples = 10; // サンプリング数。多いほど滑らかなだけど重い
    const float kBlurWidth = 0.01f; // ぼかし幅。大きいほど強いぼかし
    
    // 中心から現在のuvに対しての方向を計算
    float2 direction = input.texcoord - kCenter;
    float3 outputColor = float3(0.0f, 0.0f, 0.0f);
    for (uint sampleIndex = 0; sampleIndex < kNumSamples; ++sampleIndex)
    {
        float2 texcoord = input.texcoord + direction * kBlurWidth * float(sampleIndex);
        outputColor.rgb += gTexture.Sample(gSampler, texcoord).rgb;
    }
    
    outputColor.rgb *= rcp(float(kNumSamples));
    
    PixelShaderOutput output;
    output.color.rgb = outputColor;
    output.color.a = 1.0f;
    return output;
}