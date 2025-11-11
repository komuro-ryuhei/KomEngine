#pragma once

// Scene
#include "Game/Scene/IScene.h"
#include "Game/Scene/SceneManager.h"

// Entity
#include "Engine/Base/3d/Skybox/Skybox.h"
#include "Game/Entity/Player/Player.h"
#include "Game/Entity/Enemy/Enemy.h"
#include "Game/Entity/Enemy/BossEnemy.h"
#include "Game/Entity/Enemy/BossMeteor.h"
#include "Game/Entity/Enemy/BossSword.h"
#include "Engine/Base/Particle/ParticleManager.h"
#include "Engine/Base/Particle/ParticleEmitter.h"
#include "Fade.h"
#include "KnockoutCameraController.h"

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

	// target
	std::unique_ptr<Sprite> leftTargetOuter_;
	std::unique_ptr<Sprite> leftTargetInner_;
	std::unique_ptr<Sprite> rightTargetOuter_;
	std::unique_ptr<Sprite> rightTargetInner_;

	// Particle・Effect
	std::unique_ptr<ParticleEmitter> emitter_ = nullptr;

private:

	// 現在選択中のポストエフェクト
	int selectedPostEffectIndex_ = 0;

	// Cameraをプレイヤーに追従させるかのフラグ
	bool isCameraFollowPlayer_ = true;

	// フェード
	std::unique_ptr<Fade> fade_ = nullptr;
	enum class Phase { kFadeIn, kMain, kFadeOut };
	Phase phase_ = Phase::kFadeIn;

private:

	// 当たり判定
	void CheckCollisions();

	// PostEffectの変更関数
	void ChangePostEffect();

	// ------------------------ メテオ耐久モード ------------------------ //
	enum class MeteorPhase { kIdle, kIntro, kShower, kOutro };
	MeteorPhase meteorPhase_ = MeteorPhase::kIdle;

	// カメラ保存＆補間
	Vector3 savedCamPos_{};
	Vector3 savedCamRot_{};
	Vector3 targetCamPosOffset_{ 0.0f, 2.0f, 0.0f }; // プレイヤー位置から少し上
	float   targetPitchUp_ = -0.45f;                 // 上向き(マイナスX回転)
	float   camLerp_ = 0.0f;
	float   camIntroTime_ = 0.6f;
	float   camOutroTime_ = 0.6f;

	// 進行管理
	float meteorModeTimer_ = 0.0f;
	float meteorModeDuration_ = 8.0f;    // 耐久時間

	// メテオ
	std::vector<std::unique_ptr<BossMeteor>> meteors_;
	float spawnInterval_ = 0.7f;
	float spawnTimer_ = 0.0f;

	// 剣
	std::unique_ptr<BossSword> sword_;
	bool swordAttack_ = false;
	float swordPhaseT_ = 0.f;

	// ---- 剣カメラフォーカス制御 ----
	bool  swordCamActive_ = false;
	float swordCamIntroT_ = 0.0f;
	float swordCamOutroT_ = 0.0f;
	float swordCamIntroDur_ = 0.25f;
	float swordCamOutroDur_ = 0.25f;
	Vector3 swordSavedPos_{}, swordSavedRot_{};
	Vector3 swordAimPoint_{};
	// ---- 剣スイープの遅延開始用 ----
	bool    swordPendingSweep_ = false;
	Vector3 swordCenter_{}, swordRight_{}, swordForward_{};
	float   swordHalfLen_ = 10.0f, swordToward_ = 2.0f, swordDuration_ = 0.5f;

	// -----  ----- //
	// 腕攻撃フォーカス用カメラ
	bool   armCamActive_ = false;
	float  armCamT_ = 0.0f;
	float  armCamIntroTime_ = 0.15f; // 寄る速さ
	float  armCamOutroTime_ = 0.20f; // 戻る速さ

	Vector3 armCamSavedPos_{};
	Vector3 armCamSavedRot_{};
	Vector3 armCamTargetPos_{};
	Vector3 armCamTargetRot_{};

	KnockoutCameraController ko_;
	bool koActive_ = false;

	bool lowHpVfxOn_ = false;

	// ターゲットシェイク
	float leftTargetShakeTime_ = 0.0f;
	float rightTargetShakeTime_ = 0.0f;
	const float targetShakeDuration_ = 0.15f;   // 揺れる時間(秒)
	const float targetShakeAmplitude_ = 12.0f;   // 揺れ幅(ピクセル)


private:

	// 内部ユーティリティ
	void StartMeteorMode();
	void UpdateMeteorMode(float dt);
	void EndMeteorMode();

	// 
	void UpdateArmTargetMarker();
};