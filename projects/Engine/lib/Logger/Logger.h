#pragma once

#include <windows.h>
#include <string>

/*==================================================================================*/
// ログ関数

class Logger {

public: // 静的メンバ関数

	static void Log(const std::string& message);
};

