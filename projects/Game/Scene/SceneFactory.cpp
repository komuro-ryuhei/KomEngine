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
	} else if (sceneName == "PARTICLE") {
		return std::make_unique<ParticleEditScene>();
	} else if (sceneName == "QR") {
		return std::make_unique<QRCodeScene>();
	}
	return nullptr;
}