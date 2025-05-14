#include "OffscreenRendering.h"

void OffscreenRendering::Init() {

	// DxCommon
	dxCommon_ = System::GetDxCommon();

	// PointLight用のマテリアルリソースを作る
	materialBufferResource_ = dxCommon_->CreateBufferResource(dxCommon_->GetDevice(), sizeof(MaterialBuffer));
	materialBufferResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialBufferData_));

	materialBufferData_->time = 0.0f;

	// PSOの初期化
	pipelineManager_ = std::make_unique<PipelineManager>();
	pipelineManager_->PSOSetting("offscreen", BlendType::BLEND_NONE);

	OffScreeenRenderTargetView();
}

void OffscreenRendering::Update() {}

void OffscreenRendering::Draw() {

	materialBufferData_->time += 0.01f;

	// オブジェクトの描画処理
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(pipelineManager_->GetRootSignature());
	dxCommon_->GetCommandList()->SetPipelineState(pipelineManager_->GetGraphicsPipelineState());
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, dxCommon_->GetGPUDescriptorHandle(dxCommon_->GetSrvDescriptorHeap(), dxCommon_->GetDescriptorSizeSRV(), 0)); // SRVの設定
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(2, materialBufferResource_->GetGPUVirtualAddress());

	// 描画
	dxCommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
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

	//
	const Vector4 kRenderTargetClearValue = {1.0f, 0.0f, 0.0f, 1.0f};
	renderTextureResource_ =
	    CreateRenderTextureResource(dxCommon_->GetDevice(), System::GetWinApp()->GetWindowWidth(), System::GetWinApp()->GetWindowHeight(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, kRenderTargetClearValue);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = dxCommon_->GetRtvDesc();
	dxCommon_->GetDevice()->CreateRenderTargetView(renderTextureResource_.Get(), &rtvDesc, renderTargetHandle_);

	// SRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC renderTextureSrvDesc{};
	renderTextureSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	renderTextureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	renderTextureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	renderTextureSrvDesc.Texture2D.MipLevels = 1;

	// SRVの生成
	dxCommon_->GetDevice()->CreateShaderResourceView(renderTextureResource_.Get(), &renderTextureSrvDesc, dxCommon_->GetSrvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart());
}

void OffscreenRendering::OffScreenShaderResourceView() {}

void OffscreenRendering::RenderToTexture() {

	renderTextureBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	renderTextureBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	renderTextureBarrier.Transition.pResource = renderTextureResource_.Get();


	// レンダーテクスチャをターゲットとして設定
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon_->GetDsvHandle();
	dxCommon_->GetCommandList()->OMSetRenderTargets(1, &renderTargetHandle_, false, &dsvHandle);

	// レンダーテクスチャをクリア
	float clearColor[] = {1.0f, 0.0f, 0.0f, 1.0f}; // 赤色でクリア
	dxCommon_->GetCommandList()->ClearRenderTargetView(renderTargetHandle_, clearColor, 0, nullptr);

	// 深度ステンシルをクリア
	dxCommon_->GetCommandList()->ClearDepthStencilView(dxCommon_->GetDsvHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// ビューポートとシザー矩形を設定

	D3D12_VIEWPORT viewport = dxCommon_->GetViewPort();
	D3D12_RECT scissorRect = dxCommon_->GetScissor();
	dxCommon_->GetCommandList()->RSSetViewports(1, &viewport);
	dxCommon_->GetCommandList()->RSSetScissorRects(1, &scissorRect);

	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void OffscreenRendering::OffscreenBarrier() {

	renderTextureBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	renderTextureBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	dxCommon_->GetCommandList()->ResourceBarrier(1, &renderTextureBarrier);
}