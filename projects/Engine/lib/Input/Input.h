#pragma once
#include <windows.h>

#define DIRECTINPUT_VERSION 0x0800
#include "dinput.h"
#include <cassert>

#include "Engine/Base/WinApp/WinApp.h"
#include "Engine/lib/ComPtr/ComPtr.h"

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

/*==================================================================================*/

/// <summary>
/// 入力
/// </summary>
class Input {

public: // メンバ関数

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns>シングルトンインスタンス</returns>
	static Input* GetInstance();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(WinApp* winApp);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// キーの押下をチェック
	/// </summary>
	bool PushKey(BYTE keyNumber);
	/// <summary>
	/// キーのトリガーをチェック
	/// </summary>
	bool TriggerKey(BYTE ketNumber);

	/// <summary>
	/// マウスボタンの押下をチェック
	/// </summary>
	bool PushMouse(int button) const;
	/// <summary>
	/// マウスボタンのトリガーをチェック
	/// </summary>
	bool TriggerMouse(int button) const;

	/// <summary>
	/// マウスの移動量を取得
	/// </summary>
	POINT GetMouseDelta() const;
	/// <summary>
	/// マウスホイールの回転量を取得
	/// </summary>
	LONG  GetWheelDelta() const;

	// マウスカーソルの現在位置（クライアント座標）
	POINT GetMousePosition() const;

	/// <summary>
	/// マウスを中央に固定する
	/// </summary>
	void  SetMouseCenterLock(bool enable); // setter
	bool  IsMouseCenterLocked() const { return centerLock_; } // getter

private:

	ComPtr<IDirectInput8> directInput = nullptr;
	ComPtr<IDirectInputDevice8> keyboard = nullptr;
	ComPtr<IDirectInputDevice8> mouse = nullptr;

	// 全キーの入力情報を取得する
	BYTE key[256] = {};
	// 前回の全キー状態
	BYTE preKey[256] = {};

	DIMOUSESTATE2 mouseState_ = {};
	DIMOUSESTATE2 prevMouseState_ = {};

	// cursor lock
	bool   centerLock_ = false;
	WinApp* winApp_ = nullptr;

	// helpers
	void CenterCursorToClient();
};