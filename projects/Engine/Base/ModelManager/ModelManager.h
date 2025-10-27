#pragma once

// C++
#include <map>
#include <string>
#include <memory>

// MyClass
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/3d/Model/Model.h"

/// <summary>
/// モデル管理クラス
/// </summary>
class ModelManager {

public:

	static ModelManager* GetInstance();

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// モデル読み込み
	/// </summary>
	/// <param name="filaPath"> ファイルパス </param>
	void LoadModel(const std::string& filaPath);

	/// <summary>
	/// モデル検索
	/// </summary>
	/// <param name="filePath"> ファイルパス </param>
	Model* FindModel(const std::string& filePath);

	/// <summary>
	/// モデルのセット
	/// </summary>
	/// <param name="filePath"> ファイルパス </param>
	void SetModel(const std::string& filePath);

private:
	ModelManager() = default;
	~ModelManager() = default;
	ModelManager(ModelManager&) = delete;
	ModelManager& operator=(ModelManager&) = delete;

public:

	static ModelManager* instance;

private:

	// モデルデータ
	std::map<std::string, std::unique_ptr<Model>> models_;
};