#pragma once

#include "Engine/lib/ComPtr/ComPtr.h"

#include "Engine/Base/System/System.h"
#include "Engine/Base/WinApp/WinApp.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"

/// <summary>
/// ImGuiの管理
/// </summary>
class ImGuiManager {

public:

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="winApp"> ウィンドウズアプリケーション </param>
	void Init(WinApp* winApp);

	/// <summary>
	/// 終了処理
	/// </summary>
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

	ComPtr<ID3D12DescriptorHeap> srvHeap_;
};