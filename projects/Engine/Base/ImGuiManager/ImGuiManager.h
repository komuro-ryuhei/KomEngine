#pragma once

#include "Engine/lib/ComPtr/ComPtr.h"

#include "Engine/Base/WinApp/WinApp.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"

// ImGuiの管理
class ImGuiManager {

public:

	void Init(WinApp* winApp, DirectXCommon* dxCommon);

	void Finalize();

	/// <summary>
	/// ImGui受付処理
	/// </summary>
	void Begin();

	/// <summary>
	/// ImGui終了処理
	/// </summary>
	void End();

	/// <summary>
	/// ImGui描画処理
	/// </summary>
	void Draw();

private:

	WinApp* winApp_ = nullptr;
	DirectXCommon* dxCommon_ = nullptr;

	ComPtr<ID3D12DescriptorHeap> srvHeap_;
};