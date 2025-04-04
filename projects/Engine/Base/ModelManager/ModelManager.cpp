#include "ModelManager.h"

ModelManager* ModelManager::instance = nullptr;

ModelManager* ModelManager::GetInstance() {
	if (instance == nullptr) {
		instance = new ModelManager;
	}
	return instance;
}

void ModelManager::Init(DirectXCommon* dxCommon) {

	// 
	dxCommon_ = dxCommon;
}

void ModelManager::Finalize() {

	delete instance;
	instance = nullptr;
}

void ModelManager::LoadModel(const std::string& filePath) {

	// 読み込み済みモデルを検索
	if (models_.contains(filePath)) {
		// 読み込み済みなら早期リターン
		return;
	}

	// モデルの生成とファイル読み込み
	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Init(dxCommon_, "Resources", filePath);

	// モデルをmapコンテナに格納
	models_.insert(std::pair(filePath, std::move(model)));
}

Model* ModelManager::FindModel(const std::string& filePath) {

	// 読み込み済みモデルを検索
	if (models_.contains(filePath)) {
		// 読み込み済みモデルを戻り値としてリターン
		return models_.at(filePath).get();
	}

	// ファイル名一致なし
	return nullptr;
}