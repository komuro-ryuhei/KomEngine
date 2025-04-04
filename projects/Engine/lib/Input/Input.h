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
	/// <param name="keyNumber">キー番号</param>
	/// <returns></returns>
	bool PushKey(BYTE keyNumber);

	/// <summary>
	/// キーのトリガーをチェック
	/// </summary>
	/// <param name="ketNumber">キー番号</param>
	/// <returns></returns>
	bool TriggerKey(BYTE ketNumber);

private:

	ComPtr<IDirectInput8> directInput = nullptr;
	ComPtr<IDirectInputDevice8> keyboard;
	// 全キーの入力情報を取得する
	BYTE key[256] = {};
	// 前回の全キー状態
	BYTE preKey[256] = {};
};