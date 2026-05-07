#pragma once

// C++
#include "fstream"
#include "sstream"
#include <d3d12.h>
#include <vector>

// MyClass
#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"
#include "Engine/Base/TextureManager/TextureManager.h"
#include "Engine/Base/WinApp/WinApp.h"
#include "Engine/lib/Math/MyMath.h"
#include "struct.h"

/// <summary>
/// スカイボックスのクラス
/// スカイボックス描画を行うクラス
/// </summary>
class Skybox {

public:
	Skybox() = default;
	~Skybox() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="filename"> ファイルネーム </param>
	void Init(const std::string& filename);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// ImGuiによるデバッグ処理
	/// </summary>
	void ImGuiDebug();

public:

	// ------------------- setter ------------------- //

	// デフォルトカメラのセット
	void SetDefaultCamera(Camera* camera);

	void SetColor(Vector4 color);

private:
	// 座標初期化
	void InitPosition();

private: // メンバ変数
	// ウィンドウズアプリケーション
	WinApp* winApp_ = nullptr;

	// バッファーリソース
	ComPtr<ID3D12Resource> vertexResource;
	// バッファーリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	// バッファーリソースの使い道を補足するバッファービュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	// マテリアルリソース
	ComPtr<ID3D12Resource> materialResource;
	// マテリアルリソース内のデータを指すポインタ
	Material* materialData = nullptr;

	std::string filename_;

	// カメラ
	Camera* camera_ = nullptr;
	Camera* defaultCamera_ = nullptr;

	// 座標変換用
	// Sprite用のTransformationMatrix用のリソースを作る
	ComPtr<ID3D12Resource> transformationMatrixResource;
	TransformationMatrix* transformationMatrixData = nullptr;

	Transform transform;
	Transform cameraTransform;

	//
	std::unique_ptr<PipelineManager> pipelineManager_ = nullptr;
};