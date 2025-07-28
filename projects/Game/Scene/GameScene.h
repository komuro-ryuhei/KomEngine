#pragma once

#include <memory>
#include <chrono>

#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"
#include "Engine/Base/2d/Sprite/Sprite.h"
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Audio/Audio.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/Particle/ParticleEmitter.h"
#include "Engine/Base/Particle/ParticleManager.h"
#include "Engine/Base/3d/Skybox/Skybox.h"
#include "Engine/Base/Loader/Loader.h"

#include "Game/Scene/IScene.h"
#include "Game/Entity/Player/Player.h"

#include "Engine/lib/Network/RankingManager.h"

class GameScene : public IScene {

	enum class GamePhase {
		FirstBattle,
		Rotate,
		SecondBattle,
		Clear
	};

	GamePhase phase_ = GamePhase::FirstBattle;

	std::chrono::steady_clock::time_point lastTime;

public:

	GameScene();
	~GameScene();

	void Init() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:
	// カメラシェイク
	// void CameraShake();
	// 当たり判定処理
	void CheckCollisions();
	// ImGuiによるデバッグ表示
	void ImGuiDebug();

private:
	// Camera
	std::unique_ptr<Camera> camera_ = nullptr;
	// Audio
	std::unique_ptr<Audio> audio_ = nullptr;
	// Skybox
	std::unique_ptr<Skybox> skybox_ = nullptr;
	// Sprite
	std::unique_ptr<Sprite> sprite_ = nullptr;
	// Model
	std::unique_ptr<Object3d> object3d_ = nullptr;
	std::unique_ptr<Object3d> glassObject_ = nullptr;

	// Particle・Effect
	std::unique_ptr<ParticleEmitter> emitter_ = nullptr;
	std::unique_ptr<ParticleEmitter> emitter2_ = nullptr;
	std::unique_ptr<ParticleEmitter> ringEmitter_ = nullptr;
	std::unique_ptr<ParticleEmitter> cylinderEmitter_ = nullptr;
	std::unique_ptr<ParticleEmitter> moonLightEffect_ = nullptr;
	std::unique_ptr<ParticleEmitter> ribbonEffect_ = nullptr;

	// Player
	std::unique_ptr<Player> player_ = nullptr;

	// Enemy
	std::vector<std::unique_ptr<Enemy>> enemies_;
	std::vector<std::unique_ptr<Object3d>> enemyObjects3d_;

	struct EnemyTrigger {
		Vector3 triggerPos;
		bool triggered = false;
	};

	std::vector<EnemyTrigger> enemyTriggers_;
	int currentTriggerIndex_ = 0;
	bool isFighting_ = false;

	// 
	std::unique_ptr<Loader> loader_ = nullptr;

	// --- 回転管理の追加 ---
	bool isRotating_ = false;
	float startRotationY_ = 0.0f;
	float targetRotationY_ = 0.0f;
	float rotateStep_ = 0.0f;       // 1フレームごとの回転量
	int rotateFrameCount_ = 0;      // 今のフレーム数
	int rotateFrameMax_ = 30;       // 30フレームで回転

	bool hasSpawnedAfterRotate_ = false;

	// nerwork(エラー出るためコメントアウト中)
	// RankingManager rankingManager;

	// 現在選択中のポストエフェクト
	int selectedPostEffectIndex_ = 0;

private:

	// PostEffectの変更関数
	void ChangePostEffect();
	
	// パーティクルのデバッグ処理
	void ParticleUpdate();
	void SpawnEnemies();
	void EnemySpawnTrigger();
};