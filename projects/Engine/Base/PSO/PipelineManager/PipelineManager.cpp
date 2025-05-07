#include "PipelineManager.h"
#include "Engine/Base/System/System.h"

ID3D12RootSignature* PipelineManager::GetRootSignature() const { return rootSignature_->GetRootSignature(); }

ID3D12PipelineState* PipelineManager::GetGraphicsPipelineState() const { return graphicsPipelineState.Get(); }

PipelineManager* PipelineManager::GetInstance() {
	static PipelineManager instance;
	return &instance;
}

void PipelineManager::ShaderCompile(const std::string& objectType) {

	if (objectType == "object3d") {
		// Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Object3D.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/Object3D.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "particle") {
		// Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Particle.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/Particle.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "sprite") {
		// Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Sprite.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/Sprite.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "offscreen") {
		// Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/GaussianFilter.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	}
}

void PipelineManager::CreatePSO(const std::string& objectType) {

	if (objectType == "offscreen") {

		HRESULT hr;

		graphicsPipelineStateDesc.pRootSignature = rootSignature_->GetRootSignature();        // RootSignature
		graphicsPipelineStateDesc.InputLayout = inputLayout_->GetInputLayout();               // InputLayout
		graphicsPipelineStateDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()}; // VertexShader
		graphicsPipelineStateDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()}; // PixelShader
		graphicsPipelineStateDesc.BlendState = blendState_->GetBlendDesc();                   // BlendState
		graphicsPipelineStateDesc.RasterizerState = rasterizer_->GetRasterizerDesc();         // RasterizerState

		// 書き込むRTVの情報
		graphicsPipelineStateDesc.NumRenderTargets = 1;
		graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		// 利用するトポロジ(形状)のタイプ。三角形
		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// どのように画面に色を打ち込むのか設定
		graphicsPipelineStateDesc.SampleDesc.Count = 1;
		graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		// DepthStencilの設定
		graphicsPipelineStateDesc.DepthStencilState.DepthEnable = false; // DepthStencilを無効化

		// 実際に生成
		hr = System::GetDxCommon()->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
		assert(SUCCEEDED(hr));
	} else {
	
		HRESULT hr;

		graphicsPipelineStateDesc.pRootSignature = rootSignature_->GetRootSignature();        // RootSignature
		graphicsPipelineStateDesc.InputLayout = inputLayout_->GetInputLayout();               // InputLayout
		graphicsPipelineStateDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()}; // VertexShader
		graphicsPipelineStateDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()}; // PixelShader
		graphicsPipelineStateDesc.BlendState = blendState_->GetBlendDesc();                   // BlendState
		graphicsPipelineStateDesc.RasterizerState = rasterizer_->GetRasterizerDesc();         // RasterizerState

		// 書き込むRTVの情報
		graphicsPipelineStateDesc.NumRenderTargets = 1;
		graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		// 利用するトポロジ(形状)のタイプ。三角形
		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// どのように画面に色を打ち込むのか設定
		graphicsPipelineStateDesc.SampleDesc.Count = 1;
		graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		// DepthStencilの設定
		graphicsPipelineStateDesc.DepthStencilState = System::GetDxCommon()->GetDepthStencilDesc();
		graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		// 実際に生成
		hr = System::GetDxCommon()->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
		assert(SUCCEEDED(hr));
	}
}

void PipelineManager::PSOSetting(const std::string& objectType) {

	compiler_->Initialize();

	ShaderCompile(objectType);

	rootSignature_->Create(objectType);

	inputLayout_->Setting(objectType);

	rasterizer_->Setting();

	blendState_->Setting(objectType);

	CreatePSO(objectType);
}