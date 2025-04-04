#include "SceneManager.h"

void SceneManager::Update() {

	if (nextScene_) {
		if (currentScene_) {
			currentScene_->Finalize();
			currentScene_.reset(); // メモリ解放
		}

		// シーンの切り替え
		currentScene_ = std::move(nextScene_);
		currentScene_->SetSceneManager(this);

		// 次のシーンの初期化
		currentScene_->Init();
	}

	// 現在シーンの更新
	currentScene_->Update();
}

void SceneManager::Draw() {

	if (currentScene_) {
		currentScene_->Draw();
	}
}

void SceneManager::SetNextScene(std::unique_ptr<IScene> nextScene) { nextScene_ = std::move(nextScene); }

void SceneManager::ChangeScene(const std::string& sceneName) {

	assert(sceneFactory_);
	assert(nextScene_ == nullptr);

	// 次シーンを生成
	nextScene_ = sceneFactory_->CreateScene(sceneName);
}
