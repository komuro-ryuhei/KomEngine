#include "SceneManager.h"

#include <chrono>
#include <sstream>
#include <Windows.h>

namespace {

	void OutputTransitionTime(const char* label, double milliseconds) {

		std::ostringstream stream;

		stream
			<< "[SceneTransition] "
			<< label
			<< ": "
			<< milliseconds
			<< " ms\n";

		Logger::Log(stream.str());
	}

}

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

		// 読み込み中もノイズ演出を動かす
		fade_.Update();

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
	{
		using Clock = std::chrono::high_resolution_clock;

		const auto totalStart = Clock::now();

		if (currentScene_) {

			const auto start = Clock::now();

			currentScene_->Finalize();
			currentScene_.reset();

			const auto end = Clock::now();

			OutputTransitionTime(
				"Finalize",
				std::chrono::duration<double, std::milli>(end - start).count()
			);
		}

		{
			const auto start = Clock::now();

			currentScene_ = sceneFactory_->CreateScene(pendingSceneName_);

			const auto end = Clock::now();

			OutputTransitionTime(
				"CreateScene",
				std::chrono::duration<double, std::milli>(end - start).count()
			);
		}

		if (currentScene_) {

			currentScene_->SetSceneManager(this);

			const auto start = Clock::now();

			currentScene_->Init();

			const auto end = Clock::now();

			OutputTransitionTime(
				"Init",
				std::chrono::duration<double, std::milli>(end - start).count()
			);
		}

		const auto totalEnd = Clock::now();

		OutputTransitionTime(
			"Swap total",
			std::chrono::duration<double, std::milli>(
				totalEnd - totalStart
			).count()
		);

		fade_.StartDataErrorOpen(0.45f);
		state_ = TransitState::FadeIn;

		break;
	}

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