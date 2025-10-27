#pragma once

#include <windows.h>
#include <string>

/// <summary>
/// 文字列操作ユーティリティ
/// </summary>
class StringUtility {

public:

	/// <summary>
	/// 文字列変換 std::string → std::wstring
	/// </summary>
	/// <param name="str"> 変換する文字列 </param>
	static std::wstring ConvertString(const std::string& str);

	/// <summary>
	/// 文字列変換 std::wstring → std::string
	/// </summary>
	/// <param name="str"> 変換する文字列 </param>
	static std::string ConvertString(const std::wstring& str);
};

