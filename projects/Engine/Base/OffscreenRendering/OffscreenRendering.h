#pragma once

#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"
#include "Engine/lib/ComPtr/ComPtr.h"
#include "Engine/lib/Math/MyMath.h"

#include <d3d12.h>
#include <memory>
#include <string>

class PipelineManager;

/// <summary>
/// オフスクリーンレンダリングクラス
/// </summary>
class OffscreenRendering {

public:

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// 描画前処理
	/// </summary>
	void PostDraw();

	/// <summary>
	/// レンダーターゲット用テクスチャリソースの生成
	/// </summary>
	/// <param name="device"> デバイス </param>
	/// <param name="width"> 横幅 </param>
	/// <param name="height"> 高さ </param>
	/// <param name="format"> フォーマット </param>
	/// <param name="clearColor"> 画面の色 </param>
	/// <returns></returns>
	ComPtr<ID3D12Resource> CreateRenderTextureResource(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format, const Vector4& clearColor);
	
	/// <summary>
	/// オフスクリーンレンダーターゲットビューの設定
	/// </summary>
	void OffScreeenRenderTargetView();

	/// <summary>
	/// オフスクリーンシェーダーリソースビューの設定
	/// </summary>
	void OffScreenShaderResourceView();

public:

	/// <summary>
	/// レンダーテクスチャに描画する
	/// </summary>
	void RenderToTexture();

	/// <summary>
	/// バリアを貼る
	/// </summary>
	void OffscreenBarrier();

public:

	// getter,setter
	void SetPostEffect(const std::string& effectName); // postEffectのセット

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

	// 画面をクリアする色
	const Vector4 kRenderTargetClearValue_ = { 0.1f, 0.25f, 0.5f, 1.0f };

	std::string currentPostEffect_ = "none";
};