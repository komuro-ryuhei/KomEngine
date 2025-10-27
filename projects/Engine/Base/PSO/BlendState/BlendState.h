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

	/// <summary>
	/// 初期化設定
	/// </summary>
	/// <param name="type"> ブレンドタイプ </param>
	void Setting(BlendType type);

	// setter
	D3D12_BLEND_DESC GetBlendDesc() const; // ブレンドステートの取得

private:
	D3D12_BLEND_DESC blendDesc{};
};