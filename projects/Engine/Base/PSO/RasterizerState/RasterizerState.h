#pragma once

#include <d3d12.h>

/// <summary>
/// RasterizerStateの設定
/// </summary>
class RasterizerState {
public:
	void Setting();

	// getter
	D3D12_RASTERIZER_DESC GetRasterizerDesc() const;

private:
	D3D12_RASTERIZER_DESC rasterizerDesc{};
};