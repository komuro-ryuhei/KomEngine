#include "PipelineManager.h"
#include "Engine/Base/System/System.h"

std::unordered_map<std::string, std::shared_ptr<PipelineManager>> PipelineManager::pipelineCache_;

ID3D12RootSignature* PipelineManager::GetRootSignature() const { return rootSignature_->GetRootSignature(); }

ID3D12PipelineState* PipelineManager::GetGraphicsPipelineState() const { return graphicsPipelineState.Get(); }

std::string PipelineManager::MakeCacheKey(const std::string& objectType, BlendType type) {

	return objectType + "_" + std::to_string(static_cast<int>(type));
}

std::shared_ptr<PipelineManager> PipelineManager::GetShared(const std::string& objectType, BlendType type) {

	const std::string key = MakeCacheKey(objectType, type);

	// すでに作成済みなら再利用
	auto it = pipelineCache_.find(key);

	if (it != pipelineCache_.end()) {
		return it->second;
	}

	// 初回だけ生成・シェーダーコンパイル
	auto pipeline = std::make_shared<PipelineManager>();
	pipeline->PSOSetting(objectType, type);

	pipelineCache_.emplace(key, pipeline);

	return pipeline;
}

const std::unordered_map<std::string, PipelineManager::ShaderPair> PipelineManager::kShaderTable = {
	{ "object3d",                  { L"./Resources/shaders/Object3D.VS.hlsl",   L"./Resources/shaders/Object3D.PS.hlsl" } },
	{ "object3d_meteorError",      { L"./Resources/shaders/Object3D.VS.hlsl",   L"./Resources/shaders/MeteorError.PS.hlsl" } },
	{ "object3d_chargeCore",       { L"./Resources/shaders/Object3D.VS.hlsl",   L"./Resources/shaders/ChargeCore.PS.hlsl" } },
	{ "object3d_gridFloor",        { L"./Resources/shaders/Object3D.VS.hlsl",   L"./Resources/shaders/GridFloor.PS.hlsl" } },
	{ "particle",                  { L"./Resources/shaders/Particle.VS.hlsl",   L"./Resources/shaders/Particle.PS.hlsl" } },
	{ "sprite",                    { L"./Resources/shaders/Sprite.VS.hlsl",     L"./Resources/shaders/Sprite.PS.hlsl" } },
	{ "skybox",                    { L"./Resources/shaders/Skybox.VS.hlsl",     L"./Resources/shaders/Skybox.PS.hlsl" } },
	{ "line",                      { L"./Resources/shaders/Line.VS.hlsl",       L"./Resources/shaders/Line.PS.hlsl" } },
	{ "posteffect_none",           { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/Fullscreen.PS.hlsl" } },
	{ "posteffect_Grayscale",      { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/Grayscale.PS.hlsl" } },
	{ "posteffect_Vignetting",     { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/Vignette.PS.hlsl" } },
	{ "posteffect_Smoothing",      { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/BoxFilter.PS.hlsl" } },
	{ "posteffect_GaussinanFilter",{ L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/GaussianFilter.PS.hlsl" } },
	{ "posteffect_Bloom",          { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/Bloom.PS.hlsl" } },
	{ "posteffect_HexBarrier",     { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/HexBarrier.PS.hlsl" } },
	{ "posteffect_SpeedLine",      { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/SpeedLine.PS.hlsl" } },
	{ "posteffect_RadialBlur",     { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/RadialBlur.PS.hlsl" } },
	{ "posteffect_Random",         { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/Random.PS.hlsl" } },
	{ "posteffect_Outline",        { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/LuminanceBasedOutline.PS.hlsl" } },
	{ "posteffect_Glitch",         { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/GlitchEffect.PS.hlsl" } },
	{ "posteffect_Pixel",          { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/PixelationEffect.PS.hlsl" } },
	{ "posteffect_ChromaticAberration",{ L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/ChromaticAberration.PS.hlsl" } },
	{ "posteffect_VHSNoise",       { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/VHSNoise.PS.hlsl" } },
	{ "posteffect_ColorInversion", { L"./Resources/shaders/Fullscreen.VS.hlsl", L"./Resources/shaders/ColorInversion.PS.hlsl" } }
};

void PipelineManager::ShaderCompile(const std::string& objectType) {

	auto it = kShaderTable.find(objectType);
	if (it == kShaderTable.end()) {
		assert(false && "Unknown shader objectType.");
		return;
	}

	const ShaderPair& shader = it->second;

	vsBlob = compiler_->CompileShader(
		shader.vsPath.c_str(),
		L"vs_6_0",
		compiler_->GetDxcUtils(),
		compiler_->GetCompiler(),
		compiler_->GetIncludeHandler()
	);
	assert(vsBlob != nullptr);

	psBlob = compiler_->CompileShader(
		shader.psPath.c_str(),
		L"ps_6_0",
		compiler_->GetDxcUtils(),
		compiler_->GetCompiler(),
		compiler_->GetIncludeHandler()
	);
	assert(psBlob != nullptr);
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
	}
	else if (objectType == "sprite") {
		// 2Dスプライト/HUD：深度なし（奥オブジェクトに隠れない）
		graphicsPipelineStateDesc.DepthStencilState.DepthEnable = FALSE;
		graphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	}
	else if (objectType == "skybox") {
		// スカイボックス：深度比較のみ、有効・書き込みなし
		graphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
		graphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		graphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	}
	else {
		// 通常3D(Object3D, Particle 等)：エンジン既定のDepth設定
		graphicsPipelineStateDesc.DepthStencilState = KomEngine::System::GetDxCommon()->GetDepthStencilDesc();
	}

	// トポロジ設定：line だけ LINE、それ以外は TRIANGLE
	if (objectType == "line") {
		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	}
	else {
		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}

	// PSO生成（全分岐共通）
	hr = KomEngine::System::GetDxCommon()->GetDevice()->CreateGraphicsPipelineState(
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
	else if (
		objectType == "object3d_meteorError" ||
		objectType == "object3d_chargeCore" ||
		objectType == "object3d_gridFloor") {
		baseType = "object3d";
	}

	rootSignature_->Create(baseType);

	inputLayout_->Setting(baseType);

	rasterizer_->Setting();

	blendState_->Setting(type);

	CreatePSO(baseType);
}