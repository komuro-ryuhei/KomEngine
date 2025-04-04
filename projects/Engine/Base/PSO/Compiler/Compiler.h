#pragma once

#include <d3d12.h>
#include <dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")

#include <string>
#include <cassert>
#include <format>

#include "Engine/lib/StringUtility/StringUtility.h"
#include "Engine/lib/Logger/Logger.h"
#include "Engine/lib/ComPtr/ComPtr.h"

/// <summary>
/// シェーダーコンパイル
/// </summary>
class Compiler {
public:

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns>シングルトンインスタンス</returns>
	static Compiler* GetInstance();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// コンパイルシェーダー関数
	/// </summary>
	/// <param name="filePath">CompilerするShaderファイルのパス</param>
	/// <param name="profile">compilerに使用するprofile</param>
	/// <param name="dxcUtils">初期化で生成したもの</param>
	/// <param name="dxcCompiler">初期化で生成したもの</param>
	/// <param name="includeHandler">初期化で生成したもの</param>
	/// <returns></returns>
	ComPtr<IDxcBlob> CompileShader(
		const std::wstring& filePath,
		const wchar_t* profile,
		IDxcUtils* dxcUtils,
		IDxcCompiler3* dxcCompiler,
		IDxcIncludeHandler* includeHandler);

	// getter
	IDxcUtils* GetDxcUtils()const;
	IDxcCompiler3* GetCompiler()const;
	IDxcIncludeHandler* GetIncludeHandler()const;

private:

	ComPtr<IDxcUtils> dxcUtils_ = nullptr;
	ComPtr<IDxcCompiler3> dxcCompiler_ = nullptr;
	ComPtr<IDxcIncludeHandler> includeHandler_ = nullptr;
};