#pragma once

// C++
#include "fstream"
#include "sstream"
#include <d3d12.h>
#include <vector>

// MyClass
#include "Engine/Base/3d/Model/Model.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/ModelManager/ModelManager.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"
#include "Engine/Base/TextureManager/TextureManager.h"
#include "Engine/Base/WinApp/WinApp.h"
#include "Engine/lib/Math/MyMath.h"
#include "struct.h"

// 3Dオブジェクト
class Object3d {

public: // メンバ関数
	Object3d() = default;
	~Object3d() = default;

	void Init(BlendType type);

	void Update();

	void Draw();

	// void PreDraw();

	void ImGuiDebug();

	// setter
	void SetModel(const std::string& filePath);
	void SetCamera(Camera* camera);
	void SetDefaultCamera(Camera* camera);
	void SetScale(const Vector3& scale);
	void SetTranslate(const Vector3& translate);
	void SetRotate(const Vector3& rotate);
	void SetTransform(const Transform& transform);

	// getter
	Vector3 GetScale() const;
	Vector3 GetRotate() const;
	Vector3 GetTranslate() const;

	// getter
	Camera* GetDefaultCamera() const;

private:
	// ウィンドウズアプリケーション
	WinApp* winApp_ = nullptr;

	// モデル
	Model* model_ = nullptr;

	// カメラ
	Camera* camera_ = nullptr;
	Camera* defaultCamera_ = nullptr;

	// 座標変換用
	// Sprite用のTransformationMatrix用のリソースを作る
	ComPtr<ID3D12Resource> transformationMatrixResource;
	TransformationMatrix* transformationMatrixData = nullptr;

	Transform transform_;
	Transform cameraTransform;

	//
	std::unique_ptr<PipelineManager> pipelineManager_ = nullptr;
};