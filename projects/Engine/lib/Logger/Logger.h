#pragma once

#include <windows.h>
#include <string>

/*==================================================================================*/
// ログ関数

class Logger {

public: // 静的メンバ関数

	/// <summary>
	/// ログ出力
	/// </summary>
	/// <param name="message"> メッセージ </param>
	static void Log(const std::string& message);
};

