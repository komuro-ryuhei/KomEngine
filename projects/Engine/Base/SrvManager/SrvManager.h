#pragma once

// MyClass
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/lib/ComPtr/ComPtr.h"

// C++
#include <cassert>


// SRV管理
class SrvManager {
public:

	SrvManager() = default;
	~SrvManager() = default;

	void Init(DirectXCommon* dxCommon);

	void PreDraw();

public:

	uint32_t Allocate();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

	// SRV生成 (テクスチャ用)
	void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT MipLevels);
	// SRV生成 (テクスチャキューブ用)
	void CreateSRVforTextureCube(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT mipLevels);
	// SRV生成 (Structured Buffer用)
	void CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);

	void SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex);

	// 最大テクスチャ数に達しているかを確認する関数
	bool CanAllocate() const;

private:

	DirectXCommon* dxCommon_ = nullptr;

	// 最大SRV数(最大テクスチャ枚数)
	static const uint32_t kMaxSRVCount_;
	// SRV用のでスクリプタサイズ
	uint32_t descriptorSize_;
	// SRV用のでスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> descriptorHeap_;

	// 次に使用するSRVインデックス
	uint32_t useIndex_ = 0;
};