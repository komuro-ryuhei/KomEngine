#pragma once

#include "Game/Scene/AbstractSceneFactory.h"

#include "Game/Scene/GameScene.h"
#include "Game/Scene/TitleScene.h"

class SceneFactory : public AbstractSceneFactory {

public:
	/// <summary>
	/// シーン生成
	/// </summary>
	/// <param name="sceneName">シーンの名前</param>
	std::unique_ptr<IScene> CreateScene(const std::string& sceneName) override;
};