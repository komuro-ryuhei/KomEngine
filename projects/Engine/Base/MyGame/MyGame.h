#pragma once

#include "Game/Scene/GameScene.h"
#include "Game/Scene/SceneManager.h"
#include "Game/Scene/TitleScene.h"

#include "Game/Scene/IScene.h"
#include "Game/Scene/SceneFactory.h"

#include <memory>

/// <summary>
/// ゲームの進行を管理するクラス
/// </summary>
class MyGame {

public:

	/// <summary>
	/// 指定したシーンを実行
	/// </summary>
	/// <param name="sceneName"> 開始するシーン </param>
	void Run(const std::string& sceneName);

private:
	// シーンファクトリー
	std::unique_ptr<AbstractSceneFactory> sceneFactory_ = nullptr;

	// std::unique_ptr<GameScene> scene_ = nullptr;
	std::unique_ptr<SceneManager> sceneManager_ = nullptr;
};