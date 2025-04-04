#pragma once

#include <d3d12.h>
#include <string>
#include <vector>

/// <summary>
/// インプットレイアウトの設定
/// </summary>
class InputLayout {
public:
	/// <summary>
	/// 設定
	/// </summary>
	void Setting(const std::string& objectType);

	// getter
	D3D12_INPUT_LAYOUT_DESC GetInputLayout() const;

private:
	// D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
};