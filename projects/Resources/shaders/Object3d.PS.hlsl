
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
    float4 color; // ライトの色
    float3 direction; // ライトの向き
    float intensity; // 輝度
};

struct PointLight
{
    float4 color; // ライトの色
    float3 position; // ライトの位置
    float intensity; // 輝度
    float radius; // 半径
    float decay; // 減衰率
};

struct SpotLight
{
    float4 color; // ライトの色
    float3 position; // ライトの位置
    float intensity; // 輝度
    float3 direction; // ライトの方向
    float distance; // 方向
    float decay; // 減衰率
    float casAngle; // 余弦
    float cosFalloffStart; // 開始の角度
};

struct Camera
{
    float3 worldPosition;
};

struct ObjectParams
{
    bool useEnvironmentMap;
    float3 _padding;
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

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gMaterial.color;
    
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    output.color = gMaterial.color * textureColor;
    
    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    
    float3 pointLightDirection = normalize(input.worldPosition - gPointLight.position);
    
    //output.color.rgb += environmentColor.rgb;
    
    if (gMaterial.enableLighting != 0)
    {
        // DirectionalLight用の変数
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        
        float3 reflectLight = reflect(gDirectionalLight.direction, normalize(input.normal));
        float3 halfVector = normalize(-gDirectionalLight.direction + toEye);
        float NDotH = dot(normalize(input.normal), halfVector);
        float RdotE = dot(reflectLight, toEye);
        float specularPow = pow(saturate(NDotH), gMaterial.shininess);
        
        // PointLight用の変数
        float3 pointLightDirection = normalize(gPointLight.position - input.worldPosition);
        float3 halfVectorPoint = normalize(pointLightDirection + toEye);
        float NDotHPoint = dot(normalize(input.normal), halfVectorPoint);
        float specularPowPoint = pow(saturate(NDotHPoint), gMaterial.shininess);
        float distance = length(gPointLight.position - input.worldPosition);
        float factor = pow(saturate(-distance / gPointLight.radius + 1.0f), gPointLight.decay);
        
        // PointLight用の変数
        float3 spotLightDirectionOnSurface = normalize(input.worldPosition - gSpotLight.position);
        float3 halfVectorSpot = normalize(spotLightDirectionOnSurface + toEye);
        float NDotHSpot = dot(normalize(input.normal), halfVectorSpot);
        float specularPowSpot = pow(saturate(NDotHSpot), gMaterial.shininess);
        float distanceSpot = length(gSpotLight.position - input.worldPosition);
        float cosAngle = dot(spotLightDirectionOnSurface, gSpotLight.direction);
        float fallOffFactor = saturate((cosAngle - gSpotLight.casAngle) / (gSpotLight.cosFalloffStart - gSpotLight.casAngle));
        float attenuationFactor = pow(saturate(-distanceSpot / gSpotLight.distance + 1.0f), gSpotLight.decay);
        
        // 拡散反射
        float3 diffuseDirectionalLight = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        // 鏡面反射
        float3 specularDirectionalLight = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * float3(1.0f, 1.0f, 1.0f);
        
        // 
        float3 diffusePointLight = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * gPointLight.intensity * factor;
        // 
        float3 specularPointLight = gPointLight.color.rgb * gPointLight.intensity * specularPowPoint * float3(1.0f, 1.0f, 1.0f) * factor;
        
        // 
        float3 diffuseSpotLight = gMaterial.color.rgb * textureColor.rgb * gSpotLight.color.rgb * gSpotLight.intensity * fallOffFactor * attenuationFactor;
        // 
        float3 specularSpotLight = gSpotLight.color.rgb * gSpotLight.intensity * specularPowSpot * float3(1.0f, 1.0f, 1.0f) * fallOffFactor * attenuationFactor;
        
        // 拡散反射 + 鏡面反射
        output.color.rgb = diffuseDirectionalLight + specularDirectionalLight + diffusePointLight + specularPointLight + diffuseSpotLight + specularSpotLight;
        // アルファは今まで通り
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    
    if (objectParam.useEnvironmentMap)
    {
        // 環境マップのサンプリング
        float3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
        float3 reflectVector = reflect(-cameraToPosition, normalize(input.normal));
        float4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectVector);
    
        output.color.rgb += environmentColor.rgb;
    }
    
    // textureのα値が0の時にPisxelを棄却
    if (textureColor.a == 0.0)
    {
        discard;
    }
    
    // textureのα値が0.5以下の時にPisxelを棄却
    if (textureColor.a <= 0.5)
    {
        discard;
    }
    
    // output.colorのα値が0の時にPisxelを棄却
    if (output.color.a == 0.0)
    {
        discard;
    }
    
    return output;
}
