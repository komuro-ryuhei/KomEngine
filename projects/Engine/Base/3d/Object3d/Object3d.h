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

/// <summary>
/// 3Dオブジェクトクラス
/// 3Dモデルの実態を描画するクラス
/// </summary>
class Object3d {

public: // メンバ関数
	Object3d() = default;
	~Object3d() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="type"> ブレンドタイプ </param>
	void Init(BlendType type);
	/// <param name="type"> シェーダータイプ </param>
	void Init(const std::string& shaderType, BlendType type);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// ImGuiでのデバッグ処理
	/// </summary>
	void ImGuiDebug(const char* ImGuiName);

	// ------------------- setter ------------------- //

	void SetModel(const std::string& filePath); // モデルのセット
	void SetCamera(Camera* camera); // カメラのセット
	void SetDefaultCamera(Camera* camera); // デフォルトカメラのセット
	void SetScale(const Vector3& scale); // スケールのセット
	void SetTranslate(const Vector3& translate); // 座標のセット
	void SetRotate(const Vector3& rotate); // 回転のセット
	void SetTransform(const Transform& transform); // トランスフォームのセット
	void SetRadius(float radius) { radius_ = radius; } // 半径のセット
	void SetColor(const Vector4& color); // 色のセット

	// ------------------- getter ------------------- //

	Vector3 GetScale() const; // スケールの取得
	Vector3 GetRotate() const; // 回転の取得
	Vector3 GetTranslate() const; // 座標の取得
	float GetRadius() const; // 半径の取得
	Vector3 GetWorldPosition() const; // ワールド座標の取得
	Camera* GetDefaultCamera() const; // デフォルトカメラの取得
	Vector4 GetColor() const; // 色の取得

	// 親子関係の追加
	void SetParent(Object3d* parent);
	Object3d* GetParent() const;
	// ワールド行列の取得（他のクラスでも使いたくなる）
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }

	// 環境マップの映り込みを設定するか
	void SetEnvironmentTexture(const std::string& filePath);

	// Blenderからの座標系かどうかを設定
	void SetFromBlender(bool flag);

private:
	// ウィンドウズアプリケーション
	WinApp* winApp_ = nullptr;

	// モデル
	Model* model_ = nullptr;

	// カメラ
	Camera* camera_ = nullptr;
	Camera* defaultCamera_ = nullptr;
	
	//パイプライン
	std::unique_ptr<PipelineManager> pipelineManager_ = nullptr;

	// 座標変換用
	ComPtr<ID3D12Resource> transformationMatrixResource;
	TransformationMatrix* transformationMatrixData = nullptr;

	// 環境マップ用変数
	struct ObjectParams {
		bool useEnvironmentMap;
		Vector3 padding_;
		Vector4 color;
	};
	ComPtr<ID3D12Resource> environmentTexture_ = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE  environmentGpuHandle_{};
	ComPtr<ID3D12Resource> objectParamResource_;
	ObjectParams* objectParamData_ = nullptr;

	// 座標情報
	Transform transform_;
	Transform cameraTransform;

	// 半径
	float radius_ = 1.0f;

private:
	bool fromBlender_ = false;

	Object3d* parent_ = nullptr;
	Matrix4x4 worldMatrix_;
};