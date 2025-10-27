#pragma once

#include <d3d12.h>

/// <summary>
/// RasterizerStateの設定
/// </summary>
class RasterizerState {

public:

	/// <summary>
	/// 初期化設定
	/// </summary>
	void Setting();

	// getter
	D3D12_RASTERIZER_DESC GetRasterizerDesc() const; // ラスタライザーステートの取得

private:
	D3D12_RASTERIZER_DESC rasterizerDesc{};
};