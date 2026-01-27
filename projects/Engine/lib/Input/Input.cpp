#include "Input.h"

/*==================================================================================*/
// インスタンス

Input* Input::GetInstance() {
	static Input instance;
	return &instance;
}

void Input::Initialize(WinApp* winApp) {

	winApp_ = winApp;

	HRESULT hr;

	// DirectInputのインスタンス生成
	hr = DirectInput8Create(winApp->GetWindowClass().hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(hr));

	// -------- Keyboard -------- //
	// キーボードデバイス生成
	hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(hr));
	hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));
	hr = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));

	// ---------- mouse ---------- //
	// マウスデバイス生成
	hr = directInput->CreateDevice(GUID_SysMouse, &mouse, NULL);
	assert(SUCCEEDED(hr));
	hr = mouse->SetDataFormat(&c_dfDIMouse2);
	assert(SUCCEEDED(hr));
	// 非排他で前面時のみ
	hr = mouse->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	assert(SUCCEEDED(hr));
}

void Input::Update() {

	// -------- Keyboard -------- //
	// 前回のキー入力を保存
	memcpy(preKey, key, sizeof(key));
	// キーボード情報取得開始
	keyboard->Acquire();
	keyboard->GetDeviceState(sizeof(key), key);

	// ---------- mouse ---------- //
	prevMouseState_ = mouseState_;
	mouse->Acquire();
	mouse->GetDeviceState(sizeof(DIMOUSESTATE2), &mouseState_);

	// 中央固定が有効なら毎フレーム中央へ戻す
	if (centerLock_) {
		CenterCursorToClient();
	}
}

// ---------------- Keyboard ---------------- //

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

// ------------------- mouse ------------------- //

bool Input::PushMouse(int button) const {

	if (button < 0 || button >= 8) return false;
	return (mouseState_.rgbButtons[button] & 0x80) != 0;
}

bool Input::TriggerMouse(int button) const {

	if (button < 0 || button >= 8) return false;
	const bool now = (mouseState_.rgbButtons[button] & 0x80) != 0;
	const bool prev = (prevMouseState_.rgbButtons[button] & 0x80) != 0;
	return (!prev && now);
}

POINT Input::GetMouseDelta() const {

	POINT p{ (LONG)mouseState_.lX, (LONG)mouseState_.lY };
	return p;
}

LONG Input::GetWheelDelta() const {

	return (LONG)mouseState_.lZ;
}

POINT Input::GetMousePosition() const {

	POINT p{};
	GetCursorPos(&p);

	if (winApp_) {
		ScreenToClient(winApp_->GetHwnd(), &p); // クライアント座標へ変換
	}

	return p;
}

void Input::SetMouseCenterLock(bool enable) {

	centerLock_ = enable;
	if (centerLock_) {
		CenterCursorToClient();
		ShowCursor(FALSE);
	} else {
		ShowCursor(TRUE);
	}
}

void Input::CenterCursorToClient() {

	if (!winApp_) return;
	RECT rc{};
	GetClientRect(winApp_->GetHwnd(), &rc);
	POINT center{ (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
	ClientToScreen(winApp_->GetHwnd(), &center);
	SetCursorPos(center.x, center.y);
}