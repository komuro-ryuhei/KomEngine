#pragma once

#include <memory>

#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"

#include "Engine/Base/2d/Sprite/Sprite.h"
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Audio/Audio.h"
#include "Engine/Base/Camera/Camera.h"

#include "Engine/Base/Particle/ParticleEmitter.h"
#include "Engine/Base/Particle/ParticleManager.h"

#include "Game/Scene/IScene.h"

#include "Game/Entity/Player/Player.h"

class GameScene : public IScene {
public:
	void Init() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:
	// カメラシェイク
	void CameraShake();
	// 当たり判定処理
	void CheckCollisions();
	// ImGuiによるデバッグ表示
	void ImGuiDebug();

private:
	// Camera
	std::unique_ptr<Camera> camera_ = nullptr;
	// Audio
	std::unique_ptr<Audio> audio_ = nullptr;

	// effect
	std::unique_ptr<ParticleEmitter> emitter_ = nullptr;
	std::unique_ptr<ParticleEmitter> emitter2_ = nullptr;
	std::unique_ptr<ParticleEmitter> ringEmitter_ = nullptr;
	std::unique_ptr<ParticleEmitter> cylinderEmitter_ = nullptr;

	// Player
	std::unique_ptr<Player> player_ = nullptr;

	// Enemy
	std::vector<std::unique_ptr<Enemy>> enemies_;
	std::vector<std::unique_ptr<Object3d>> enemyObjects3d_;

	// カメラシェイク
	float shakeTimer_ = 0.0f;
	float shakeDuration_ = 0.5f; // 秒数
	bool isShaking_ = false;
	Vector3 cameraDefaultPos_;


	struct EnemyTrigger {
		float triggerZ; // プレイヤーのZ座標がここを超えたら出現
		bool triggered = false;
	};

	std::vector<EnemyTrigger> enemyTriggers_;
	int currentTriggerIndex_ = 0;
	bool isFighting_ = false;

private:
	void ParticleUpdate();
	void SpawnEnemies();
	void EnemySpawnTrigger();
};