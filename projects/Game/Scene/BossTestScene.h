#pragma once

// Scene
#include "Game/Scene/IScene.h"

// Entity
#include "Engine/Base/3d/Skybox/Skybox.h"
#include "Game/Entity/Player/Player.h"
#include "Game/Entity/Enemy/Enemy.h"
#include "Game/Entity/Enemy/BossEnemy.h"

class BossTestScene : public IScene {
public:

	BossTestScene() = default;
	~BossTestScene() = default;

	void Init() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:
	// Camera
	std::unique_ptr<Camera> camera_ = nullptr;
	// 
	std::unique_ptr<Object3d> glassObject_ = nullptr;

	// Skybox
	std::unique_ptr<Skybox> skybox_ = nullptr;

	// Player
	std::unique_ptr<Player> player_ = nullptr;
	// Boss
	std::unique_ptr<BossEnemy> boss_ = nullptr;

	// 現在選択中のポストエフェクト
	int selectedPostEffectIndex_ = 0;

	// Cameraをプレイヤーに追従させるかのフラグ
	bool isCameraFollowPlayer_ = true;

private:

	// 当たり判定
	void CheckCollisions();

	// PostEffectの変更関数
	void ChangePostEffect();
};