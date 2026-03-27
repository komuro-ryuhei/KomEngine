#include "AssetPreloader.h"
#include "Engine/Base/System/System.h"
#include "Engine/Base/TextureManager/TextureManager.h"
#include "Engine/Base/ModelManager/ModelManager.h"

#include <fstream>

using json = nlohmann::json;

void AssetPreloader::Clear() {
	textures_.clear();
	models_.clear();
	texSet_.clear();
	modelSet_.clear();
	texIndex_ = 0;
	modelIndex_ = 0;
	started_ = false;
}

void AssetPreloader::AddTexture(const std::string& path) {
	if (texSet_.insert(path).second) {
		textures_.push_back(path);
	}
}

void AssetPreloader::AddModel(const std::string& name) {
	if (modelSet_.insert(name).second) {
		models_.push_back(name);
	}
}

bool AssetPreloader::BuildListFromJson(const std::string& jsonPath, const std::string& sceneName) {
	Clear();

	std::ifstream ifs(jsonPath);
	if (!ifs.is_open()) {
		return false;
	}

	json root;
	ifs >> root;

	if (!root.contains("scenes")) {
		return false;
	}

	const auto& scenes = root["scenes"];
	if (!scenes.contains(sceneName)) {
		return false;
	}

	const auto& scene = scenes[sceneName];

	if (scene.contains("textures") && scene["textures"].is_array()) {
		for (const auto& tex : scene["textures"]) {
			if (tex.is_string()) {
				AddTexture(tex.get<std::string>());
			}
		}
	}

	if (scene.contains("models") && scene["models"].is_array()) {
		for (const auto& model : scene["models"]) {
			if (model.is_string()) {
				AddModel(model.get<std::string>());
			}
		}
	}

	return true;
}

// 古い呼び出し互換用
void AssetPreloader::BuildListFor(const std::string& sceneName) {
	// JSONから読めなかった時の保険にしてもよい
	BuildListFromJson("Resources/json/assetPreload.json", sceneName);
}

void AssetPreloader::Start() {
	started_ = true;
	texIndex_ = 0;
	modelIndex_ = 0;
}

bool AssetPreloader::BuildCommonListFromJson(const std::string& jsonPath) {
	Clear();

	std::ifstream ifs(jsonPath);
	if (!ifs.is_open()) {
		return false;
	}

	json root;
	ifs >> root;

	if (root.contains("textures") && root["textures"].is_array()) {
		for (const auto& tex : root["textures"]) {
			if (tex.is_string()) {
				AddTexture(tex.get<std::string>());
			}
		}
	}

	if (root.contains("models") && root["models"].is_array()) {
		for (const auto& model : root["models"]) {
			if (model.is_string()) {
				AddModel(model.get<std::string>());
			}
		}
	}

	return true;
}

void AssetPreloader::Update(int budgetPerFrame) {
	if (!started_) return;

	int budget = budgetPerFrame;

	while (budget > 0 && texIndex_ < textures_.size()) {
		KomEngine::System::GetTextureManager()->LoadTexture(textures_[texIndex_++]);
		--budget;
	}
	while (budget > 0 && modelIndex_ < models_.size()) {
		ModelManager::GetInstance()->LoadModel(models_[modelIndex_++]);
		--budget;
	}
}

bool AssetPreloader::IsDone() const {
	return started_
		&& texIndex_ >= textures_.size()
		&& modelIndex_ >= models_.size();
}

float AssetPreloader::GetProgress() const {
	const float total = float(textures_.size() + models_.size());
	if (total <= 0.0f) return 1.0f;
	const float done = float(texIndex_ + modelIndex_);
	return done / total;
}