#pragma once

#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"
#include "Engine/lib/Input/Input.h"

#include "Game/Scene/AbstractSceneFactory.h"
#include "Game/Scene/IScene.h"
#include <memory> // std::unique_ptr

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
	// シーンファクトリー
	AbstractSceneFactory* sceneFactory_ = nullptr;

	// 今のシーン
	std::unique_ptr<IScene> currentScene_ = nullptr;
	// 次のシーン
	std::unique_ptr<IScene> nextScene_ = nullptr;
};