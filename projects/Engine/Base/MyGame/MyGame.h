#pragma once

#include "Game/Scene/GameScene.h"
#include "Game/Scene/SceneManager.h"
#include "Game/Scene/TitleScene.h"

#include "Game/Scene/IScene.h"
#include "Game/Scene/SceneFactory.h"

#include <memory>

class MyGame {

public:
	void Run();

private:
	// シーンファクトリー
	std::unique_ptr<AbstractSceneFactory> sceneFactory_ = nullptr;

	// std::unique_ptr<GameScene> scene_ = nullptr;
	std::unique_ptr<SceneManager> sceneManager_ = nullptr;
};