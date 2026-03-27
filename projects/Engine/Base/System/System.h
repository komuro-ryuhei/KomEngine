#pragma once

#include <cassert>
#include <cstdint>
#include <format>
#include <memory>
#include <string>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

// MyClass
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/Mesh/Light.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"
#include "Engine/Base/WinApp/WinApp.h"
#include "Engine/Base/OffscreenRendering/OffscreenRendering.h"
#include "Engine/lib/ComPtr/ComPtr.h"
#include "Engine/lib/Input/Input.h"
#include "Engine/lib/Logger/Logger.h"
#include "Engine/Base/SrvManager/SrvManager.h"
#include "Engine/Base/TextureManager/TextureManager.h"
#include "Engine/Base/Particle/ParticleManager.h"

/// <summary>
/// システム
/// </summary>
class System {

public: // 静的メンバ関数
	/// <summary>
	/// システム全体の初期化
	/// </summary>
	/// <param name="title">タイトル</param>
	/// <param name="width">ウィンドウの高さ</param>
	/// <param name="height">ウィンドウの幅</param>
	static void Initialize(const char* title, int width, int height);

	/// <summary>
	/// システム全体の更新
	/// </summary>
	static void Update();

	/// <summary>
	/// Windowsのメッセージを処理する
	/// </summary>
	/// <returns>1: ゲーム終了 0: ゲーム継続</returns>
	static bool ProcessMessage();

	/// <summary>
	/// フレーム開始処理
	/// </summary>
	static void BeginFrame();

	/// <summary>
	/// フレーム終了処理
	/// </summary>
	static void EndFrame();

	/// <summary>
	/// 終了処理
	/// </summary>
	static void Finalize();

public:

	/// <summary>
	/// プッシュキー
	/// </summary>
	static bool PushKey(BYTE keyNumber);

	/// <summary>
	/// トリガーキー
	/// </summary>
	static bool TriggerKey(BYTE keyNumber);

public:
	/// <summary>
	/// getter・setter
	/// </summary>
	static float GetDeltaTime();                        // デルタタイムの取得
	static DirectXCommon* GetDxCommon();                // DxCommonの取得
	static Input* GetInput();                           // Inputの取得
	static SrvManager* GetSrvManager();                 // SrvManagerの取得
	static TextureManager* GetTextureManager();         // TextureManagerの取得
	static ParticleManager* GetParticleManager();       // ParticleManagerの取得
	static Light* GetLight();                           // Lightの取得
	static WinApp* GetWinApp();                         // WinAppの取得
	static OffscreenRendering* GetOffscreenRendering(); // OffscreenRenderingの取得
};