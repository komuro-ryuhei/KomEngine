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

class Sprite {

public:
	Sprite() = default;
	~Sprite() = default;

	void Init(const std::string& textureFilePath, BlendType type);

	void Update();

	void Draw();

	void PreDraw();

	/// <summary>
	/// ImGuiのでバグ表示
	/// </summary>
	void ImGuiDebug();

public:
	// getter
	const Vector2& GetPosition() const;
	float GetRotation() const;
	const Vector4& GetColor() const;
	const Vector2& GetSize() const;
	const Vector2& GetAnchorPoint() const;
	const bool& GetIsFilpX() const;
	const bool& GetIsFilpY() const;
	const Vector2& GetTextureLeftTop() const;
	const Vector2& GetTextureSize() const;
	// setter
	void SetPosition(const Vector2& position);
	void SetRotation(float rotation);
	void SetColor(const Vector4& color);
	void SetSize(const Vector2& size);
	void SetTexture(const std::string& textureFilePath);
	void SetAnchorPoint(const Vector2& anchorPoint);
	void SetIsFlipX(const bool& isFlipX);
	void SetIsFlipY(const bool& isFlipY);
	void SetTextureLeftTop(const Vector2& textureLeftTop);
	void SetTextureSize(const Vector2& textureSize);

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