#include "RasterizerState.h"

D3D12_RASTERIZER_DESC RasterizerState::GetRasterizerDesc() const { return rasterizerDesc; }

void RasterizerState::Setting() {

	// 背面カリング
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
}