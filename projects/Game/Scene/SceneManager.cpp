#include "SceneManager.h"

void SceneManager::Update() {

	if (state_ == TransitState::Idle) {
		if (currentScene_) currentScene_->Update();
		return;
	}

	switch (state_) {
	case TransitState::FadeOut:
		fade_.Update();
		if (fade_.IsFinished()) {
			state_ = TransitState::Loading;
		}
		break;

	case TransitState::Loading:
		if (preloader_) {
			preloader_->Update(loadBudgetPerFrame_);
			if (preloader_->IsDone()) {
				state_ = TransitState::Swap;
			}
		} else {
			state_ = TransitState::Swap;
		}
		break;

	case TransitState::Swap:
		if (currentScene_) {
			currentScene_->Finalize();
			currentScene_.reset();
		}

		currentScene_ = sceneFactory_->CreateScene(pendingSceneName_);
		currentScene_->SetSceneManager(this);

		// ここは軽くする（Init内でLoadしない前提）
		currentScene_->Init();

		fade_.StartDataErrorOpen(0.45f);
		state_ = TransitState::FadeIn;
		break;

	case TransitState::FadeIn:
		fade_.Update();
		if (fade_.IsFinished()) {
			fade_.Stop();
			state_ = TransitState::Idle;
		}
		break;

	default:
		break;
	}
}

void SceneManager::Draw() {

	if (currentScene_) currentScene_->Draw();

	if (state_ != TransitState::Idle) {
		fade_.Draw();
	}
}

void SceneManager::SetNextScene(std::unique_ptr<IScene> nextScene) { nextScene_ = std::move(nextScene); }

void SceneManager::ChangeScene(const std::string& sceneName) {

	assert(sceneFactory_);

	if (state_ != TransitState::Idle) return;

	pendingSceneName_ = sceneName;

	if (!preloader_) {
		preloader_ = std::make_unique<AssetPreloader>();
	}

	if (!commonAssetsLoaded_) {
		preloader_->BuildCommonListFromJson("Resources/json/assetPreload.json");
		preloader_->Start();
		commonAssetsLoaded_ = true;
	} else {
		preloader_->Clear();
		preloader_->Start();
	}

	fade_.Initialize(1280, 720);
	fade_.StartDataErrorClose(0.6f);

	state_ = TransitState::FadeOut;
}