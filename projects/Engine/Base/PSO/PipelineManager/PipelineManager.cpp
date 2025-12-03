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
	} else if (objectType == "line") {
		// line 用 Shader をコンパイル
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Line.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/Line.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "posteffect_none") {
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
	} else if (objectType == "posteffect_Outline") {
		// Random用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/LuminanceBasedOutline.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "posteffect_Glitch") {
		// Random用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/GlitchEffect.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "posteffect_Pixel") {
		// Random用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/PixelationEffect.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "posteffect_ChromaticAberration") {
		// Random用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/ChromaticAberration.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "posteffect_VHSNoise") {
		// Random用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/VHSNoise.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	} else if (objectType == "posteffect_ColorInversion") {
		// Random用Shaderをコンパイルする
		vsBlob = compiler_->CompileShader(L"./Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(vsBlob != nullptr);
		psBlob = compiler_->CompileShader(L"./Resources/shaders/ColorInversion.PS.hlsl", L"ps_6_0", compiler_->GetDxcUtils(), compiler_->GetCompiler(), compiler_->GetIncludeHandler());
		assert(psBlob != nullptr);
	}
}

void PipelineManager::CreatePSO(const std::string& objectType)
{
	HRESULT hr = S_OK;

	// 一旦クリアして毎回フル設定（他の分岐と統一）
	graphicsPipelineStateDesc = {};

	// 共通設定
	graphicsPipelineStateDesc.pRootSignature = rootSignature_->GetRootSignature();
	graphicsPipelineStateDesc.InputLayout = inputLayout_->GetInputLayout();
	graphicsPipelineStateDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	graphicsPipelineStateDesc.BlendState = blendState_->GetBlendDesc();
	graphicsPipelineStateDesc.RasterizerState = rasterizer_->GetRasterizerDesc();
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; // 共通DSV

	// 種類別設定
	if (objectType == "posteffect") {
		// オフスクリーン/フルスクリーン用：深度なし
		graphicsPipelineStateDesc.DepthStencilState.DepthEnable = FALSE;
		graphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	} else if (objectType == "sprite") {
		// 2Dスプライト/HUD：深度なし（奥オブジェクトに隠れない）
		graphicsPipelineStateDesc.DepthStencilState.DepthEnable = FALSE;
		graphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	} else if (objectType == "skybox") {
		// スカイボックス：深度比較のみ、有効・書き込みなし
		graphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
		graphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		graphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	} else {
		// 通常3D(Object3D, Particle 等)：エンジン既定のDepth設定
		graphicsPipelineStateDesc.DepthStencilState = System::GetDxCommon()->GetDepthStencilDesc();
	}

	// トポロジ設定：line だけ LINE、それ以外は TRIANGLE
	if (objectType == "line") {
		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	} else {
		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}

	// PSO生成（全分岐共通）
	hr = System::GetDxCommon()->GetDevice()->CreateGraphicsPipelineState(
		&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState)
	);
	assert(SUCCEEDED(hr));
}

void PipelineManager::PSOSetting(const std::string& objectType, BlendType type) {

	compiler_->Initialize();

	ShaderCompile(objectType);

	// shaderがposteffectの時にposteffectの設定に変更する
	std::string baseType = objectType;
	if (objectType.find("posteffect_") == 0) {
		baseType = "posteffect";
	}

	rootSignature_->Create(baseType);

	inputLayout_->Setting(baseType);

	rasterizer_->Setting();

	blendState_->Setting(type);

	CreatePSO(baseType);
}