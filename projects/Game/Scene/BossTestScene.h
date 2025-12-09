#pragma once

// Scene
#include "Game/Scene/IScene.h"
#include "Game/Scene/SceneManager.h"
#include "Engine/Base/Particle/ParticleManager.h"
#include "Engine/Base/Particle/ParticleEmitter.h"
#include "LineRenderer.h"

// Entity
#include "Engine/Base/3d/Skybox/Skybox.h"
#include "Game/Entity/Player/Player.h"
#include "Game/Entity/Enemy/Enemy.h"
#include "Game/Entity/Enemy/BossEnemy.h"
#include "Game/Entity/Enemy/BossMeteor.h"
#include "Game/Entity/Enemy/BossSword.h"

#include "Fade.h"
#include "KnockoutCameraController.h"
#include "ResultImage.h"
#include "BossMeteorController.h"
#include "BossArmController.h"
#include "CollisionManager.h"

class BossTestScene : public IScene {

public:

	enum class BossAttackType {
		Arms,
		Meteor,
		Sword,
	};

	// AABB を LineRenderer で描画するヘルパー
	inline void AddAABBLines(LineRenderer& line, const AABB& box, const Vector4& color)
	{
		const Vector3& mn = box.min;
		const Vector3& mx = box.max;

		// 8頂点
		Vector3 v000{ mn.x, mn.y, mn.z };
		Vector3 v100{ mx.x, mn.y, mn.z };
		Vector3 v010{ mn.x, mx.y, mn.z };
		Vector3 v110{ mx.x, mx.y, mn.z };

		Vector3 v001{ mn.x, mn.y, mx.z };
		Vector3 v101{ mx.x, mn.y, mx.z };
		Vector3 v011{ mn.x, mx.y, mx.z };
		Vector3 v111{ mx.x, mx.y, mx.z };

		// 下側の四角
		line.AddLine(v000, v100, color);
		line.AddLine(v100, v110, color);
		line.AddLine(v110, v010, color);
		line.AddLine(v010, v000, color);

		// 上側の四角
		line.AddLine(v001, v101, color);
		line.AddLine(v101, v111, color);
		line.AddLine(v111, v011, color);
		line.AddLine(v011, v001, color);

		// 縦の4本
		line.AddLine(v000, v001, color);
		line.AddLine(v100, v101, color);
		line.AddLine(v110, v111, color);
		line.AddLine(v010, v011, color);
	}

	BossAttackType currentAttackType_ = BossAttackType::Meteor;

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

	// デバッグ用ライン描画
	LineRenderer debugLine_;

	// Player
	std::unique_ptr<Player> player_ = nullptr;
	// Playerが持つ銃
	std::unique_ptr<Object3d> gun_ = nullptr;
	// カメラからの相対位置・回転
	Vector3 gunOffset_{ 0.0f, -1.0f, 3.0f }; // 右/左, 上下, 前
	Vector3 gunRotOffset_{ 0.0f, 1.5f, 0.0f }; // カメラからの回転オフセット

	// Boss
	std::unique_ptr<BossEnemy> boss_ = nullptr;

	// リザルトのスプライト
	std::unique_ptr<ResultImage> result_ = nullptr;

	// target
	std::unique_ptr<Sprite> leftTargetOuter_;
	std::unique_ptr<Sprite> leftTargetInner_;
	std::unique_ptr<Sprite> rightTargetOuter_;
	std::unique_ptr<Sprite> rightTargetInner_;

	// Particle・Effect
	std::unique_ptr<ParticleEmitter> emitter_ = nullptr;

	// 当たり判定管理
	CollisionManager collisionManager_;

private:

	// 現在選択中のポストエフェクト
	int selectedPostEffectIndex_ = 0;

	// Cameraをプレイヤーに追従させるかのフラグ
	bool isCameraFollowPlayer_ = true;

	// フェード
	std::unique_ptr<Fade> fade_ = nullptr;
	enum class Phase { kFadeIn, kMain, kFadeOut };
	Phase phase_ = Phase::kFadeIn;

	// シーン終了理由
	enum class EndReason {
		None,
		PlayerDeath,
		BossDeath,
	};
	EndReason endReason_ = EndReason::None;

private:

	// PostEffectの変更関数
	void ChangePostEffect();

private:

	// メテオ
	std::vector<std::unique_ptr<BossMeteor>> meteors_;

	// メテオ攻撃の制御はコントローラに委譲
	std::unique_ptr<BossMeteorController> meteorController_;

	// 腕攻撃コントローラ（カメラ演出含む）
	std::unique_ptr<BossArmController> armController_;

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

	KnockoutCameraController ko_;
	bool koActive_ = false;

	bool lowHpVfxOn_ = false;

	// ★ ボス撃破後 → 着地してからの待ち時間用
	float bossDeathTimer_ = 0.0f;

	// ターゲットシェイク
	float leftTargetShakeTime_ = 0.0f;
	float rightTargetShakeTime_ = 0.0f;
	const float targetShakeDuration_ = 0.15f;   // 揺れる時間(秒)
	const float targetShakeAmplitude_ = 12.0f;   // 揺れ幅(ピクセル)


private:

	// アームの位置に描画するマーカーの更新
	void UpdateArmTargetMarker();

	// 銃の更新用
	void UpdateGun();

	// メテオ関連の制御（入力・ボスリクエスト・カメラ追従のON/OFF）
	void UpdateMeteorControl(float dt);

	// 
	void LineTarget();
};