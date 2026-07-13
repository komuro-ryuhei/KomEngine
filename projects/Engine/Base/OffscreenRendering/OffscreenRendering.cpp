#include "OffscreenRendering.h"
#include "Engine/Base/System/System.h"

// getter,setter
void OffscreenRendering::SetPostEffect(const std::string& effectName) {

	if (currentPostEffect_ == effectName) {
		return;
	}

	// 作成済みならキャッシュを再利用、
	// 初回だけシェーダーとPSOを生成する
	pipelineManager_ = PipelineManager::GetShared(
		"posteffect_" + effectName,
		BlendType::BLEND_NONE
	);

	currentPostEffect_ = effectName;
}

void OffscreenRendering::SetPostEffectParam(float param0, float param1, float param2, float param3) {

	if (!materialBufferData_) {
		return;
	}

	materialBufferData_->param0 = param0;
	materialBufferData_->param1 = param1;
	materialBufferData_->param2 = param2;
	materialBufferData_->param3 = param3;
}

void OffscreenRendering::Init() {

	// ポストエフェクト用マテリアルバッファを生成
	materialBufferResource_ =
		KomEngine::System::GetDxCommon()->CreateBufferResource(
			KomEngine::System::GetDxCommon()->GetDevice(),
			sizeof(MaterialBuffer)
		);

	materialBufferResource_->Map(
		0,
		nullptr,
		reinterpret_cast<void**>(&materialBufferData_)
	);

	materialBufferData_->time = 0.0f;

	materialBufferData_->param0 = 0.0f;
	materialBufferData_->param1 = 0.0f;
	materialBufferData_->param2 = 0.0f;
	materialBufferData_->param3 = 0.0f;

	// 起動時は通常のポストエフェクトを設定
	pipelineManager_ = PipelineManager::GetShared(
		"posteffect_none",
		BlendType::BLEND_NONE
	);

	currentPostEffect_ = "none";

	// オフスクリーン用レンダーターゲット生成
	OffScreeenRenderTargetView();
}

void OffscreenRendering::Update() {}

void OffscreenRendering::Draw() {

	materialBufferData_->time += 0.01f;

	// オブジェクトの描画処理
	KomEngine::System::GetDxCommon()->GetCommandList()->SetGraphicsRootSignature(pipelineManager_->GetRootSignature());
	KomEngine::System::GetDxCommon()->GetCommandList()->SetPipelineState(pipelineManager_->GetGraphicsPipelineState());
	KomEngine::System::GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(
	    1, KomEngine::System::GetDxCommon()->GetGPUDescriptorHandle(KomEngine::System::GetDxCommon()->GetSrvDescriptorHeap(), KomEngine::System::GetDxCommon()->GetDescriptorSizeSRV(), 0)); // SRVの設定
	KomEngine::System::GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(2, materialBufferResource_->GetGPUVirtualAddress());

	// 描画
	KomEngine::System::GetDxCommon()->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void OffscreenRendering::PostDraw() {

	// バリアの設定
	renderTextureBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	renderTextureBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	KomEngine::System::GetDxCommon()->GetCommandList()->ResourceBarrier(1, &renderTextureBarrier);
}

ComPtr<ID3D12Resource> OffscreenRendering::CreateRenderTextureResource(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format, const Vector4& clearColor) {

	// 生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;                                   // Textureの幅
	resourceDesc.Height = height;                                 // Textureの高さ
	resourceDesc.MipLevels = 1;                                   // mipmapの数
	resourceDesc.DepthOrArraySize = 1;                            //
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;        // DepthStencilとして利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1;                            // サンプリングカウント
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // DepthStencilとして使う通知

	//
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	//
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = format;
	clearValue.Color[0] = clearColor.x;
	clearValue.Color[1] = clearColor.y;
	clearValue.Color[2] = clearColor.z;
	clearValue.Color[3] = clearColor.w;

	ComPtr<ID3D12Resource> resource = nullptr;

	device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&resource));

	return resource;
}

void OffscreenRendering::OffScreeenRenderTargetView() {

	renderTargetHandle_ = KomEngine::System::GetDxCommon()->GetRtvStartHandle();
	renderTargetHandle_.ptr += KomEngine::System::GetDxCommon()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * 2;

	//
	const Vector4 kRenderTargetClearValue = {kRenderTargetClearValue_};
	renderTextureResource_ = CreateRenderTextureResource(
	    KomEngine::System::GetDxCommon()->GetDevice(), KomEngine::System::GetWinApp()->GetWindowWidth(), KomEngine::System::GetWinApp()->GetWindowHeight(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, kRenderTargetClearValue);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = KomEngine::System::GetDxCommon()->GetRtvDesc();
	KomEngine::System::GetDxCommon()->GetDevice()->CreateRenderTargetView(renderTextureResource_.Get(), &rtvDesc, renderTargetHandle_);

	// SRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC renderTextureSrvDesc{};
	renderTextureSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	renderTextureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	renderTextureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	renderTextureSrvDesc.Texture2D.MipLevels = 1;

	// SRVの生成
	KomEngine::System::GetDxCommon()->GetDevice()->CreateShaderResourceView(
	    renderTextureResource_.Get(), &renderTextureSrvDesc, KomEngine::System::GetDxCommon()->GetSrvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart());
}

void OffscreenRendering::OffScreenShaderResourceView() {}

void OffscreenRendering::RenderToTexture() {

	renderTextureBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	renderTextureBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	renderTextureBarrier.Transition.pResource = renderTextureResource_.Get();

	// レンダーテクスチャをターゲットとして設定
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = KomEngine::System::GetDxCommon()->GetDsvHandle();
	KomEngine::System::GetDxCommon()->GetCommandList()->OMSetRenderTargets(1, &renderTargetHandle_, false, &dsvHandle);

	// レンダーテクスチャをクリア
	float clearColor[] = {kRenderTargetClearValue_.x, kRenderTargetClearValue_.y, kRenderTargetClearValue_.z, kRenderTargetClearValue_.w};
	KomEngine::System::GetDxCommon()->GetCommandList()->ClearRenderTargetView(renderTargetHandle_, clearColor, 0, nullptr);

	// 深度ステンシルをクリア
	KomEngine::System::GetDxCommon()->GetCommandList()->ClearDepthStencilView(KomEngine::System::GetDxCommon()->GetDsvHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// ビューポートとシザー矩形を設定
	D3D12_VIEWPORT viewport = KomEngine::System::GetDxCommon()->GetViewPort();
	D3D12_RECT scissorRect = KomEngine::System::GetDxCommon()->GetScissor();
	KomEngine::System::GetDxCommon()->GetCommandList()->RSSetViewports(1, &viewport);
	KomEngine::System::GetDxCommon()->GetCommandList()->RSSetScissorRects(1, &scissorRect);

	KomEngine::System::GetDxCommon()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void OffscreenRendering::OffscreenBarrier() {

	renderTextureBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	renderTextureBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	KomEngine::System::GetDxCommon()->GetCommandList()->ResourceBarrier(1, &renderTextureBarrier);
}