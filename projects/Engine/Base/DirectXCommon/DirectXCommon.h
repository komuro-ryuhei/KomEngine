#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>

#include <cassert>
#include <chrono>
#include <thread>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include "externals/DirectXTex/DirectXTex.h"

#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/PSO/Compiler/Compiler.h"
#include "Engine/Base/WinApp/WinApp.h"
#include "Engine/lib/StringUtility/StringUtility.h"
#include "Engine/lib/ComPtr/ComPtr.h"
#include "Engine/lib/Math/MyMath.h"

/// <summary>
/// DirectXCommon
/// </summary>
class DirectXCommon {
public: // 静的メンバ変数
	DirectXCommon() = default;
	~DirectXCommon() = default;

	/// <summary>
	/// シングルトンインスタンス
	/// </summary>
	/// <returns></returns>
	static DirectXCommon* GetInstance();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(WinApp* winApp);

	/// <summary>
	/// 描画前処理
	/// </summary>
	void PreDraw();

	/// <summary>
	/// 描画後処理
	/// </summary>
	void PostDraw();

	/// <summary>
	/// レンダーターゲットのクリア
	/// </summary>
	void CrearRenderTargets();

	/// <summary>
	/// リソース作成
	/// </summary>
	ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

	/// <summary>
	/// ディスクリプターヒープの生成
	/// </summary>
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heaptype, UINT numDescriptors, bool shaderVisible);

	void CreateDescriptorHeaps();

	/// <summary>
	/// getter
	/// </summary>
	ID3D12Device* GetDevice() const;

	ID3D12GraphicsCommandList* GetCommandList() const;

	D3D12_VIEWPORT GetViewPort() const;

	D3D12_RECT GetScissor() const;

	ID3D12DescriptorHeap* GetSrvDescriptorHeap();

	uint32_t GetDescriptorSizeSRV() const;
	uint32_t GetDescriptorSizeRTV() const;
	uint32_t GetDescriptorSizeDSV() const;

	D3D12_DEPTH_STENCIL_DESC GetDepthStencilDesc() const;

size_t GetBackBufferCount() const;

private: // メンバ変数
	// ウィンドウサイズ
	uint32_t kWindowWidth_;
	uint32_t kWindowHeight_;

	// ウィンドウズアプリケーション
	WinApp* winApp_;

	// DiretcX
	ComPtr<IDXGIFactory7> dxgiFactory_;
	ComPtr<IDXGIAdapter4> useAdapter_;
	ComPtr<ID3D12Device> device_;
	// command
	ComPtr<ID3D12CommandQueue> commandQueue_;
	ComPtr<ID3D12CommandAllocator> commandAllocator_;
	ComPtr<ID3D12GraphicsCommandList> commandList_;

	// スワップチェイン
	ComPtr<IDXGISwapChain4> swapChain_;
	ComPtr<ID3D12Resource> swapChainResources[2] = {nullptr};
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

	// ディスクリプターヒープ
	ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

	// RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	ComPtr<ID3D12InfoQueue> infoQueue_ = nullptr;

	ComPtr<ID3D12Fence> fence_;
	uint64_t fenceValue = 0;
	HANDLE fenceEvent;

	D3D12_RESOURCE_BARRIER barrier{};

	// ビューポート
	D3D12_VIEWPORT viewPort{};
	// シザー矩形
	D3D12_RECT scissorRect{};

	// 記録時間(FPS固定用)
	std::chrono::steady_clock::time_point reference_;

	// DescriptorSize
	uint32_t descriptorSizeSRV;
	uint32_t descriptorSizeRTV;
	uint32_t descriptorSizeDSV;

	ComPtr<ID3D12Resource> depthStencilResource;
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;

	// 
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle_;

public:

	static const uint32_t kMaxSRVCount;

private: // メンバ関数
	DirectXCommon(const DirectXCommon&) = delete;
	const DirectXCommon& operator=(const DirectXCommon&) = delete;

	/// <summary>
	/// デバイス関連初期化
	/// </summary>
	void InitializeDXGIDevice();

	/// <summary>
	/// コマンド関連初期化
	/// </summary>
	void InitializeCommand();

	/// <summary>
	/// Descriptorのサイズ設定
	/// </summary>
	void SettingDescriptorSize();

	/// <summary>
	/// スワップチェーンの作成
	/// </summary>
	void CreateSwapChain();

	/// <summary>
	/// レンダーターゲットの生成
	/// </summary>
	void CreateRenderTargets();

	/// <summary>
	/// バリアを張る
	/// </summary>
	void TransitionBarrier();

	/// <summary>
	/// フェンスの作成
	/// </summary>
	void CreateFence();

	/// <summary>
	/// シザリング矩形の初期化
	/// </summary>
	void InitializeScissoringRect();

	/// <summary>
	/// FPS固定初期化
	/// </summary>
	void InitializeFixFPS();

	/// <summary>
	/// FPS固定更新
	/// </summary>
	void UpdateFixFPS();

	/// <summary>
	/// DSVの初期化
	/// </summary>
	void InitializeDepthStencilView();

	// 
	ComPtr<ID3D12Resource> CreateRenderTextureResource(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format, const Vector4& clearColor);
	// 
	void OffScreeenRenderTargetView();
	// 
	void OffScreenShaderResourceView();

public:

	void RenderToTexture();


public:
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	DirectX::ScratchImage LoadTexture(const std::string& filePath);

	ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

	void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

	ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);
};