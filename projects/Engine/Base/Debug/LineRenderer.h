#pragma once

// C++
#include <d3d12.h>

// MyClass
#include "Engine/Base/System/System.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"
#include "Engine/lib/Math/MyMath.h"
#include "struct.h"

// 前方宣言（Camera クラス）
class Camera;

/// <summary>
/// 3Dライン描画クラス
/// デバッグラインなどの描画を行う
/// </summary>
class LineRenderer {

public:
	LineRenderer() = default;
	~LineRenderer() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	void Init(uint32_t maxLines, BlendType type);

	/// <summary>
	/// 毎フレーム更新（カメラ行列の反映など）
	/// </summary>
	void Update();

	/// <summary>
	/// ライン追加
	/// </summary>
	void AddLine(const Vector3& start, const Vector3& end, const Vector4& color);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// 全ライン削除（描画後に自動でも呼ばれる）
	/// </summary>
	void Clear();

	/// <summary>
	/// 使用するカメラのセット
	/// </summary>
	void SetCamera(Camera* camera);

private:
	struct LineVertex {
		Vector3 position;
		Vector4 color;
	};

	struct LineCameraMatrix {
		Matrix4x4 viewProj;
	};

private:

	std::shared_ptr<PipelineManager> pipelineManager_ = nullptr;

	// 頂点バッファ
	ComPtr<ID3D12Resource> vertexResource_;
	LineVertex* vertexData_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	// カメラ用CB
	ComPtr<ID3D12Resource> cameraResource_;
	LineCameraMatrix* cameraData_ = nullptr;

	// 頂点数管理
	uint32_t maxVertices_ = 0;
	uint32_t vertexCount_ = 0;

	// 使用カメラ
	Camera* camera_ = nullptr;
};
