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
		// objects用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Object3D.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/Object3D.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "particle") {
		// particle用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Particle.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/Particle.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "sprite") {
		// sprite用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Sprite.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/Sprite.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "skybox") {
		// slybox用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Skybox.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/Skybox.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "offscreen") {
		// offscreen用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "posteffect_Grayscale") {
		// Grayscale用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/Grayscale.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "posteffect_Vignetting") {
		// Vignetting用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/Vignette.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "posteffect_Smoothing") {
		// Smoothing用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/BoxFilter.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "posteffect_GaussinanFilter") {
		// GaussinanFilter用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/GaussianFilter.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "posteffect_RadialBlur") {
		// RadialBlur用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/RadialBlur.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "posteffect_Random") {
		// Random用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/Random.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	}
}

void PipelineManager::CreatePSO(const std::string& objectType) {

	if (objectType == "posteffect") {

		HRESULT hr;

		graphicsPipelineStateDesc.pRootSignature = rootSignature_->GetRootSignature();        // RootSignature
		graphicsPipelineStateDesc.InputLayout = inputLayout_->GetInputLayout();               // InputLayout
		graphicsPipelineStateDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() }; // VertexShader
		graphicsPipelineStateDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() }; // PixelShader
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
	} else if (objectType == "skybox") {

		HRESULT hr;

		graphicsPipelineStateDesc.pRootSignature = rootSignature_->GetRootSignature();        // RootSignature
		graphicsPipelineStateDesc.InputLayout = inputLayout_->GetInputLayout();               // InputLayout
		graphicsPipelineStateDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() }; // VertexShader
		graphicsPipelineStateDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() }; // PixelShader
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
		graphicsPipelineStateDesc.DepthStencilState.DepthEnable = true;                           // DepthStencilを有効化
		graphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // Depthを全て書き込む
		graphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // Depthの比較方法
		graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		// 実際に生成
		hr = System::GetDxCommon()->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
		assert(SUCCEEDED(hr));
	} else {

		HRESULT hr;

		graphicsPipelineStateDesc.pRootSignature = rootSignature_->GetRootSignature();        // RootSignature
		graphicsPipelineStateDesc.InputLayout = inputLayout_->GetInputLayout();               // InputLayout
		graphicsPipelineStateDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() }; // VertexShader
		graphicsPipelineStateDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() }; // PixelShader
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

void PipelineManager::PSOSetting(const std::string& objectType, BlendType type) {

	compiler_->Initialize();

	ShaderCompile(objectType);

	// shaderがposteffectの時にposteffectの設定に変更する
	std::string baseType = objectType;
	if (objectType.find("posteffect_") == 0 || objectType == "offscreen") {
		baseType = "posteffect";
	}

	rootSignature_->Create(baseType);

	inputLayout_->Setting(baseType);

	rasterizer_->Setting();

	blendState_->Setting(type);

	CreatePSO(baseType);
}