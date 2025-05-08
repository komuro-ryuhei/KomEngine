#pragma once

#include <d3d12.h>
#include <string>

enum BlendType {
	BLEND_NONE,
	BLEND_ALPHA,
	BLEND_ADD,
	BLEND_MULTIPLY,
	BLEND_SUBTRACT,
};

/// <summary>
/// ブレンドステートの設定
/// </summary>
class BlendState {
public:
	void Setting(BlendType type);

	// setter
	D3D12_BLEND_DESC GetBlendDesc() const;

private:
	D3D12_BLEND_DESC blendDesc{};
};