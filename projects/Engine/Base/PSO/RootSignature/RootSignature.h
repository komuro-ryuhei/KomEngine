#pragma once

#include <d3d12.h>

#include "Engine/lib/ComPtr/ComPtr.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/lib/Logger/Logger.h"

#include <string>

class RootSignature {
public:
	/// <summary>
	/// 生成
	/// </summary>
	void Create(const std::string& objectName);

	// getter
	ID3D12RootSignature* GetRootSignature() const;

	RootSignature() = default;
	~RootSignature() = default;

private:
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	ComPtr<ID3DBlob> signatureBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
};