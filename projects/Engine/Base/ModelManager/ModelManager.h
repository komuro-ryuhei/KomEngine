#pragma once

// C++
#include <map>
#include <string>
#include <memory>

// MyClass
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/3d/Model/Model.h"

class ModelManager {

public:

	static ModelManager* GetInstance();

	void Init(DirectXCommon* dxCommon);

	void Finalize();

	void LoadModel(const std::string& filaPath);
	Model* FindModel(const std::string& filePath);

	void SetModel(const std::string& filePath);

private:
	ModelManager() = default;
	~ModelManager() = default;
	ModelManager(ModelManager&) = delete;
	ModelManager& operator=(ModelManager&) = delete;

public:
	static ModelManager* instance;

private:

	DirectXCommon* dxCommon_ = nullptr;

	// モデルデータ
	std::map<std::string, std::unique_ptr<Model>> models_;
};