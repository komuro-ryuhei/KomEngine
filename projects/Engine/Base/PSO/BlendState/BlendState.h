#pragma once

#include <d3d12.h>

/// <summary>
/// ブレンドステートの設定
/// </summary>
class BlendState {
public:

	void Setting();

	// setter
	D3D12_BLEND_DESC GetBlendDesc() const;

private:
	D3D12_BLEND_DESC blendDesc{};
};