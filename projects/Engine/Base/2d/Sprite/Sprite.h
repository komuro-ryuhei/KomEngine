#pragma once

// C++
#include <d3d12.h>

// MyClass
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"
#include "Engine/Base/TextureManager/TextureManager.h"
#include "Engine/Base/WinApp/WinApp.h"
#include "Engine/lib/Math/MyMath.h"
#include "struct.h"

/// <summary>
/// Spriteクラス
/// 2Dスプライト描画を行うクラス
/// </summary>
class Sprite {

public:
	Sprite() = default;
	~Sprite() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="textureFilePath"> ファイルパス </param>
	/// <param name="type"> ブレンドタイプ </param>
	void Init(const std::string& textureFilePath, BlendType type);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// 描画前処理
	/// </summary>
	void PreDraw();

	/// <summary>
	/// ImGuiのでバグ表示
	/// </summary>
	void ImGuiDebug();

public:
	// ------------------- getter ------------------- //

	const Vector2& GetPosition() const; // 座標
	const Vector2 GetCenterPosition() const; // 中心座標
	float GetRotation() const; // 回転角
	const Vector4& GetColor() const; // 色
	const Vector2& GetSize() const; // サイズ
	const Vector2& GetAnchorPoint() const; // アンカーポイント
	const bool& GetIsFilpX() const; // 左右フリップ
	const bool& GetIsFilpY() const; // 上下フリップ
	const Vector2& GetTextureLeftTop() const; // テクスチャ左上座標
	const Vector2& GetTextureSize() const; // テクスチャ切り出しサイズ

	// ------------------- setter ------------------- //

	void SetPosition(const Vector2& position); // 座標
	void SetRotation(float rotation); // 回転角
	void SetColor(const Vector4& color); // 色
	void SetSize(const Vector2& size); // サイズ
	void SetTexture(const std::string& textureFilePath); // テクスチャ設定
	void SetAnchorPoint(const Vector2& anchorPoint); // アンカーポイント
	void SetIsFlipX(const bool& isFlipX); // 左右フリップ
	void SetIsFlipY(const bool& isFlipY); // 上下フリップ
	void SetTextureLeftTop(const Vector2& textureLeftTop); // テクスチャ左上座標
	void SetTextureSize(const Vector2& textureSize); // テクスチャ切り出しサイズ

	// マウスとスプライトの当たり判定
	bool HitTest(const Vector2& mousePos) const;

private:
	// テクスチャサイズをイメージに合わせる
	void AdjustTextureSize();

private:
	// ウィンドウズアプリケーション
	WinApp* winApp_ = nullptr;
	// PSO
	std::unique_ptr<PipelineManager> pipelineManager_ = nullptr;

	// バッファーリソース
	ComPtr<ID3D12Resource> vertexResource;
	ComPtr<ID3D12Resource> indexResource;
	// バッファーリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	uint32_t* indexData = nullptr;
	// バッファーリソースの使い道を補足するバッファービュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	// マテリアルリソース
	ComPtr<ID3D12Resource> materialResource;
	// テクスチャリソース
	ComPtr<ID3D12Resource> textureResource;
	// マテリアルリソース内のデータを指すポインタ
	Material* materialData = nullptr;
	// 座標変換用
	// Sprite用のTransformationMatrix用のリソースを作る
	ComPtr<ID3D12Resource> transformationMatrixResource;
	TransformationMatrix* transformationMatrixData = nullptr;

	Transform transform;
	Transform cameraTransform;

	Transform uvTransform;

	// RTV
	ID3D12DescriptorHeap* rtvDescriptorHeap;
	// SRV用のヒープでディスクリプタの数は128
	ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU;

private:
	uint32_t textureIndex = 0;

	std::string textureFilePath_;

	Vector2 position_ = {0.0f, 0.0f};
	float rotation_ = 0.0f;
	Vector2 size_ = {640.0f, 360.0f};

	Vector2 anchorPoint_ = {0.0f, 0.0f};

	// テクスチャ左上座標
	Vector2 textureLeftTop_ = {0.0f, 0.0f};
	// テクスチャ切り出しサイズ
	Vector2 textureSize_ = {64.0f, 64.0f};

	// 左右フリップ
	bool isFlipX_ = false;
	bool isFlipY_ = false;
};