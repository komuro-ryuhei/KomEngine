#include "Input.h"

/*==================================================================================*/
// インスタンス

Input* Input::GetInstance() {
	static Input instance;
	return &instance;
}

void Input::Initialize(WinApp* winApp) {

	HRESULT hr;

	// DirectInputのインスタンス生成
	hr = DirectInput8Create(winApp->GetWindowClass().hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(hr));
	// キーボードデバイス生成
	hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(hr));
	// 入力データの形式セット
	hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));
	// 排他制御レベルのセット
	hr = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));
}

void Input::Update() {

	// 前回のキー入力を保存
	memcpy(preKey, key, sizeof(key));

	// キーボード情報取得開始
	keyboard->Acquire();
	keyboard->GetDeviceState(sizeof(key), key);
}

bool Input::PushKey(BYTE keyNumber) {

	// 指定キーを押していたらtrue
	if (key[keyNumber]) {
		return true;
	}

	// それ以外はfalse
	return false;
}

bool Input::TriggerKey(BYTE keyNumber) {
	
	// 指定キーをトリガーしていたらtrue
	if (!preKey[keyNumber] && key[keyNumber]) {
		return true;
	}

	// それ以外はfalse
	return false;
}