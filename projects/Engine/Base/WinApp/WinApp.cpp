#include "WinApp.h"

#include "externals/imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

const wchar_t WinApp::kWindowName[] = L"GE3";

/*==================================================================================*/
// getter
WNDCLASS WinApp::GetWindowClass() const { return wndClass_; }
HWND WinApp::GetHwnd() const { return hwnd_; }

/*==================================================================================*/
// インスタンス

WinApp* WinApp::GetInstance() {
	static WinApp instance;
	return &instance;
}

int32_t WinApp::GetWindowWidth() { return kWindowWidth_; }

int32_t WinApp::GetWindowHeight() { return kWindowHeight_; }

/*==================================================================================*/
// メッセージ

bool WinApp::ProcessMessage() {

	MSG msg{};

	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) // メッセージがあるか
	{
		TranslateMessage(&msg); // キー入力メッセージの処理
		DispatchMessage(&msg);  // ウィンドウプロシージャにメッセージを送る
	}

	if (msg.message == WM_QUIT) // 終了メッセージが来たらループを抜ける
	{
		return true;
	}

	return false;
}

/*==================================================================================*/
// ウインドウプロシージャ

LRESULT WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
		return true;

	// メッセージに応じてゲーム固有の処理
	switch (msg) {
		// ウインドウが破棄された
	case WM_DESTROY:
		// OSに対してアプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

/*==================================================================================*/
// ウィンドウ作成

void WinApp::CreateGameWindow(const wchar_t* title, UINT windowStyle, int32_t clientWidth, int32_t clientHeight) {

	windowStyle_ = windowStyle;
	aspectRatio_ = float(clientWidth) / float(clientHeight);

	// COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	// システムタイマーの分解能を上げる
	timeBeginPeriod(1);

	// ウインドウプロシージャ
	wndClass_.lpfnWndProc = WindowProc;
	// ウインドウクラス名
	wndClass_.lpszClassName = kWindowName;
	// インスタンスハンドル
	wndClass_.hInstance = GetModuleHandle(nullptr);
	// カーソル
	wndClass_.hCursor = LoadCursor(nullptr, IDC_ARROW);

	// ウインドウクラスを登録する
	RegisterClass(&wndClass_);

	// ウインドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = {0, 0, clientWidth, clientHeight};

	// クライアント領域を元に実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウインドの生成
	hwnd_ = CreateWindow(wndClass_.lpszClassName, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, wrc.right - wrc.left, wrc.bottom - wrc.top, nullptr, nullptr, wndClass_.hInstance, nullptr);

	// ウインドウを表示する
	ShowWindow(hwnd_, SW_SHOW);
}

void WinApp::TerminateGameWindow() {
	// 
	UnregisterClass(wndClass_.lpszClassName, wndClass_.hInstance);
}