#include "SceneFactory.h"

std::unique_ptr<IScene> SceneFactory::CreateScene(const std::string& sceneName) {

	if (sceneName == "TITLE") {
		return std::make_unique<TitleScene>();
	} else if (sceneName == "GAME") {
		return std::make_unique<GameScene>();
	} else if (sceneName == "TEST") {
		return std::make_unique<BossTestScene>();
	} else if (sceneName == "GAMEOVER") {
		return std::make_unique<GameOverScene>();
	}
	return nullptr;
}