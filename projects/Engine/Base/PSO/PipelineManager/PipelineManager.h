#pragma once

#include "Engine/Base/PSO/Compiler/Compiler.h"

// MyClass
#include "Engine/Base/PSO/BlendState/BlendState.h"
#include "Engine/Base/PSO/Inputlayout/InputLayout.h"
#include "Engine/Base/PSO/RootSignature/RootSignature.h"
#include "Engine/Base/PSO/RasterizerState/RasterizerState.h"

#include "Engine/lib/ComPtr/ComPtr.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"

#include <string>
#include <memory>

class RootSignature;

/// <summary>
/// パイプライン管理
/// </summary>
class PipelineManager {
public:
	PipelineManager() = default;
	~PipelineManager() = default;

	/// <summary>
	/// シングルトンインスタンス
	/// </summary>
	/// <returns></returns>
	static PipelineManager* GetInstance();

	/// <summary>
	/// ShaderをCompile
	/// </summary>
	void ShaderCompile(const std::string& objectType);

	/// <summary>
	/// PSOを生成
	/// </summary>
	void CreatePSO(const std::string& objectType);

	void PSOSetting(const std::string& objectType, BlendType type);

	ID3D12RootSignature* GetRootSignature() const;
	ID3D12PipelineState* GetGraphicsPipelineState() const;

private:
	std::unique_ptr<Compiler> compiler_ = std::make_unique<Compiler>();
	std::unique_ptr<RootSignature> rootSignature_ = std::make_unique<RootSignature>();
	std::unique_ptr<InputLayout> inputLayout_ = std::make_unique<InputLayout>();
	std::unique_ptr<RasterizerState> rasterizer_ = std::make_unique<RasterizerState>();
	std::unique_ptr<BlendState> blendState_ = std::make_unique<BlendState>();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

	ComPtr<IDxcBlob> vsBlob;
	ComPtr<IDxcBlob> psBlob;

private:
	PipelineManager(const PipelineManager&) = delete;
	const PipelineManager& operator=(const PipelineManager&) = delete;
};
