#pragma once
#include <Windows.h>
#include <cstdint>

#pragma comment(lib,"winmm.lib")

#include "Engine/lib/ComPtr/ComPtr.h"

/// <summary>
/// ウィンドウズアプリケーション
/// </summary>
class WinApp {
public: // 静的メンバ変数

	// ウィンドウサイズ
	static const int32_t kWindowWidth_ = 1280;
	static const int32_t kWindowHeight_ = 720;
	// ウィンドウクラス名
	static const wchar_t kWindowName[];

public: // メンバ関数

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns>シングルトンインスタンス</returns>
	static WinApp* GetInstance();

	/// <summary>
	/// ウィンドウサイズの横幅の取得
	/// </summary>
	int32_t GetWindowWidth();

	/// <summary>
	/// ウィンドウサイズの高さの取得
	/// </summary>
	int32_t GetWindowHeight();

	/// <summary>
	/// メッセージ
	/// </summary>
	/// <returns></returns>
	bool ProcessMessage();

	/// <summary>
	/// ウインドウプロシージャ
	/// </summary>
	/// <param name="hwnd">ウインドウハンドル</param>
	/// <param name="msg">メッセージ番号</param>
	/// <param name="wparam">メッセージ情報1</param>
	/// <param name="lpalam">メッセージ情報2</param>
	/// <returns></returns>
	static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	/// <summary>
	/// ゲームウインドウの作成
	/// </summary>
	/// <param name="title">ウインドウタイトル</param>
	/// <param name="windowStyle">ウインドウの初期スタイル</param>
	/// <param name="clientWidth">ウインドウのクライアントの横幅</param>
	/// <param name="clientHeight">ウインドウのクライアントの縦幅</param>
	void CreateGameWindow(const wchar_t* title = L"DirectXGame", UINT windowStyle = WS_OVERLAPPEDWINDOW,
		int32_t clientWidth = kWindowWidth_, int32_t clientHeight = kWindowHeight_);
	
	/// <summary>
	/// ゲームウィンドウの破棄
	/// </summary>
	void TerminateGameWindow();

	WNDCLASS GetWindowClass() const;

	HWND GetHwnd() const;

private: // メンバ変数

	HWND hwnd_; // ウィンドウハンドル
	WNDCLASS wndClass_{}; // ウィンドウクラス
	UINT windowStyle_;
	float aspectRatio_;

public:// メンバ関数

	WinApp() = default;
	~WinApp() = default;
	WinApp(const WinApp&) = delete;
	const WinApp& operator=(const WinApp&) = delete;
};