#pragma once

#include "Engine/lib/ComPtr/ComPtr.h"
#include "Engine/lib/Math/MyMath.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"

#include <d3d12.h>
#include <memory>

class PipelineManager;

class OffscreenRendering {

public:

	void Init();

	void Update();

	void Draw();

	void PostDraw();

	//
	ComPtr<ID3D12Resource> CreateRenderTextureResource(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format, const Vector4& clearColor);
	//
	void OffScreeenRenderTargetView();
	//
	void OffScreenShaderResourceView();

public:
	void RenderToTexture();

	void OffscreenBarrier();

private:

	// DxCommon
	// DirectXCommon* dxCommon_ = nullptr;

	// PSO
	std::unique_ptr<PipelineManager> pipelineManager_ = nullptr;

	// バリア
	D3D12_RESOURCE_BARRIER renderTextureBarrier{};

	//
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle_;

	//
	ComPtr<ID3D12Resource> renderTextureResource_;

	// Random用のマテリアルリソースを作る
	ComPtr<ID3D12Resource> materialBufferResource_;
	MaterialBuffer* materialBufferData_ = nullptr;
};