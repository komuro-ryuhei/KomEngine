#pragma once

#include <d3d12.h>

#include "Engine/lib/Vector/Vector.h"

// MyClass
#include "Engine/lib/ComPtr/ComPtr.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/lib/Math/MyMath.h"

#include "struct.h"

/// <summary>
/// メッシュ
/// </summary>
class Light {
public:

	struct DirectionalLight {
		Vector4 color;
		Vector3 direction;
		float intensity;
	};

	struct PointLight {
		Vector4 color;    // ライトの色
		Vector3 position; // ライトの位置
		float intensity;  // 輝度
		float radius;     // 半径
		float decay;      // 減衰率
		float padding[2]; // パディング
	};

	struct SpotLight {
		Vector4 color;     // ライトの色
		Vector3 position;  // ライトの位置
		float intensity;   // 輝度
		Vector3 direction; // ライトの方向
		float distance;    // 距離
		float decay;       // 減衰率
		float casAngle;    // 余弦
		float cosFalloffStart; // 開始の角度
		float padding[2];  // パディング
	};

	struct CameraForGPU {
		Vector3 worldPosition;
	};

	void LightSetting();

	void ImGuiDebug();

	ID3D12Resource* GetLightResource()const;
	ID3D12Resource* GetPhongLightResource()const;
	ID3D12Resource* GetPointLightResource()const;
	ID3D12Resource* GetSpotLightResource()const;

private:
	// Light用のマテリアルリソースを作る
	ComPtr<ID3D12Resource> materialResourceLight;
	DirectionalLight* directionalLightData = nullptr;

	// Phong用のマテリアルリソースを作る
	ComPtr<ID3D12Resource> materialResourcePhong;
	CameraForGPU* phongLightData = nullptr;

	// PointLight用のマテリアルリソースを作る
	ComPtr<ID3D12Resource> materialResourcePoint;
	PointLight* pointLightData = nullptr;

	// SpotLight用のマテリアルリソースを作る
	ComPtr<ID3D12Resource> materialResourceSpot;
	SpotLight* spotLightData = nullptr;
};