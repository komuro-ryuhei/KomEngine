#pragma once

#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"
#include "Engine/lib/Input/Input.h"
#include "Game/UI/Fade.h"
#include "AssetPreloader.h" 

#include "Game/Scene/AbstractSceneFactory.h"
#include "Game/Scene/IScene.h"
#include <memory>

class SceneManager {

public:

	~SceneManager() = default;

	void Update();

	void Draw();

	// 次シーン予約
	void SetNextScene(std::unique_ptr<IScene> nextScene);

	void ChangeScene(const std::string& sceneName);

	// シーンファクトリーのsetter
	void SetSceneFactory(AbstractSceneFactory* sceneFactory) { sceneFactory_ = sceneFactory; }

private:
	
	// シーン遷移の状態
	enum class TransitState { Idle, FadeOut, Loading, Swap, FadeIn };
	TransitState state_ = TransitState::Idle;

	// シーンファクトリー
	AbstractSceneFactory* sceneFactory_ = nullptr;

	// 今のシーン
	std::unique_ptr<IScene> currentScene_ = nullptr;
	// 次のシーン
	std::unique_ptr<IScene> nextScene_ = nullptr;

	// 遷移予定のシーン名（デバッグ用）
	std::string pendingSceneName_;

	// 遷移演出をSceneManagerが持つ
	Fade fade_;

	// アセットプリローダー
	std::unique_ptr<AssetPreloader> preloader_;
	int loadBudgetPerFrame_ = 4;
};