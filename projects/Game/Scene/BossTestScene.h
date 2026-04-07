#pragma once

// Scene
#include "Game/Scene/IScene.h"
#include "Game/Scene/SceneManager.h"
#include "Engine/Base/Particle/ParticleManager.h"
#include "Engine/Base/Particle/ParticleEmitter.h"
#include "Engine/Base/Debug/LineRenderer.h"

// Object
#include "Engine/Base/3d/Skybox/Skybox.h"
#include "Engine/Base/2d/Sprite/Sprite.h"

// Entity
#include "Game/Entity/Player/Player.h"
#include "Game/Entity/Enemy/Enemy.h"
#include "Game/Entity/Enemy/BossEnemy.h"
#include "Game/Entity/Enemy/BossMeteor.h"
#include "Game/Entity/Enemy/BossSword.h"
#include "Game/Entity/Enemy/BossMissile.h"

#include "Game/UI/Fade.h"
#include "Game/Camera/KnockoutCameraController.h"
#include "Game/UI/ResultImage.h"
#include "Game/GamePlay/Enemy/BossMeteorController.h"
#include "Game/GamePlay/Enemy/BossArmController.h"
#include "Engine/Base/Collision/CollisionManager.h"
#include "Game/UI/PauseMenu.h"

#include "Game/GamePlay/Enemy/BossAttackManager.h"

#include <vector>

class BossTestScene : public IScene {

public:

	bool lowHpVfxOn_ = false;

	// ポストエフェクトのデバッグモード
	enum class PostEffectDebugMode {
		Auto = 0,            // 低HP時だけビネット（今までの挙動）
		None,
		Grayscale,
		Vignetting,
		Smoothing,
		GaussinanFilter,     // 綴りは PipelineManager に合わせる
		RadialBlur,
		Random,
		Outline,
		Glitch,
		Pixel,
		ChromaticAberration,
		VHSNoise,
		ColorInversion,
	};

	PostEffectDebugMode postEffectDebugMode_ = PostEffectDebugMode::Auto;

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

	BossTestScene() = default;
	~BossTestScene() = default;

	void Init() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

	void ImGuiDebug();


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

	// 操作方法スプライト
	std::unique_ptr<Sprite> controlGuideSprite_ = nullptr;
	std::unique_ptr<Sprite> controlGuideSprite2_ = nullptr;
	// 
	std::unique_ptr<Sprite> toPauseSpr_;

	// リザルトのスプライト
	std::unique_ptr<ResultImage> result_ = nullptr;

	// target
	std::unique_ptr<Sprite> leftTargetOuter_;
	std::unique_ptr<Sprite> leftTargetInner_;
	std::unique_ptr<Sprite> rightTargetOuter_;
	std::unique_ptr<Sprite> rightTargetInner_;

	std::vector<std::unique_ptr<Sprite>> missileTelegraphMarkers_;

	// Particle・Effect
	std::unique_ptr<ParticleEmitter> emitter_ = nullptr;

	// 当たり判定管理
	CollisionManager collisionManager_;

	// 攻撃管理
	std::unique_ptr<BossAttackManager> attackManager_;

	// Play開始時の基準
	Vector3 playCameraPos_;
	Vector3 playCameraRot_;
	Vector3 bossPlayPos_;

private:

	// Cameraをプレイヤーに追従させるかのフラグ
	bool isCameraFollowPlayer_ = true;

	// フェード
	// std::unique_ptr<Fade> fade_ = nullptr;
	enum class Phase { kFadeIn, kMain, kFadeOut };
	Phase phase_ = Phase::kFadeIn;

	enum class GameFlowState { Intro, Play };
	GameFlowState flowState_ = GameFlowState::Intro;

	// シーン終了理由
	enum class EndReason {
		None,
		BossDeath,
		PlayerDeath,
		GoTitle,
	};
	EndReason endReason_ = EndReason::None;

private:

	// カメラ更新関数
	void UpdateCamera(float dt);

	// PostEffectの変更関数
	void ChangePostEffect();

	// プレイヤー死亡処理
	void UpdatePlayerDeath(float dt);

	// ノックアウト開始ヘルパー
	void StartKnockout(int fallSide);

private:

	// メテオ
	std::vector<std::unique_ptr<BossMeteor>> meteors_;

	std::vector<std::unique_ptr<BossMissile>> missiles_;

	//// メテオ攻撃の制御はコントローラに委譲
	//std::unique_ptr<BossMeteorController> meteorController_;

	//// 腕攻撃コントローラ（カメラ演出含む）
	//std::unique_ptr<BossArmController> armController_;

	KnockoutCameraController ko_;
	bool koActive_ = false;
	// ノックアウト演出が終わった後も、フェードアウトまでカメラを固定する
	bool koFrozen_ = false;

	// ボス撃破後 → 着地してからの待ち時間用
	float bossDeathTimer_ = 0.0f;

	// ターゲットシェイク
	float leftTargetShakeTime_ = 0.0f;
	float rightTargetShakeTime_ = 0.0f;
	const float targetShakeDuration_ = 0.15f;  // 揺れる時間(秒)
	const float targetShakeAmplitude_ = 12.0f; // 揺れ幅(ピクセル)

	// 演出パラメータ（あとでjson化してOK）
	float introCamLookUpTime_ = 0.6f;   // 上を見る時間
	float introFallStartDelay_ = 0.2f;  // 少し溜めて落とす
	float introFollowStrength_ = 6.0f;  // カメラ追従強さ
	float bossStartHeight_ = 40.0f;     // 出現高度
	float bossGroundY_ = 0.0f;          // 地面
	float bossFallSpeed_ = 30.0f;       // 落下速度（演出用）

	enum class IntroPhase { CamIn, Falling, CamOut };
	IntroPhase introPhase_ = IntroPhase::CamIn;

	Vector3 introSavedCamPos_{};
	Vector3 introSavedCamRot_{};
	float   introCamLerp_ = 0.0f;

	// 着地シェイク
	float landingShakeTime_ = 0.35f;
	float landingShakePower_ = 0.35f;
	bool landingTriggered_ = false;
	float landingTimer_ = 0.0f;
	float landingWaitTime_ = 0.35f;

	bool collisionEnabled_ = false;
	
	// カメラ追従の強さ（0.0f なら即座にプレイヤー位置、1.0f なら追従なし）
	float cameraPosLerp_ = 0.10f;
	float cameraRotLerp_ = 0.15f;

	// チャージ中の一時注視
	bool chargeLookActive_ = false;
	float chargeLookBlend_ = 0.0f;
	float chargeLookInSpeed_ = 5.0f;
	float chargeLookOutSpeed_ = 6.0f;

	// コアそのものだと少し低い場合の微調整
	Vector3 chargeLookOffset_{ 0.0f, 1.2f, 0.0f };

	bool chargeMarkerBreakActive_ = false;
	float chargeMarkerBreakTimer_ = 0.0f;
	float chargeMarkerBreakDuration_ = 0.18f;
	Vector2 chargeMarkerBreakScreenPos_{};

	// ポーズ用
	std::unique_ptr<PauseMenu> pauseMenu_;

	// PlayerのHPのUI表示（後にクラス分け予定）
	std::vector<std::unique_ptr<Sprite>> hpHearts_;
	int playerMaxHp_ = 5;                 // 現状プレイヤーHPは 5
	Vector2 hpStartPos_{ 20.0f, 700.0f }; // 左下
	float hpHeartInterval_ = 52.0f;       // ハートの間隔
	Vector2 hpHeartSize_{ 64.0f, 64.0f }; // ハートのサイズ

	// イントロ終了後のキラーン演出
	std::unique_ptr<Sprite> bossIntroGlintSprite_ = nullptr;
	bool bossIntroGlintActive_ = false;
	float bossIntroGlintTimer_ = 0.0f;
	float bossIntroGlintDuration_ = 0.45f;

	// ボス中央付近から少し上にずらす
	Vector3 bossIntroGlintOffset_{ 0.0f, 1.2f, 0.0f };

	bool playStartPending_ = false; // キラーン後に戦闘開始する待機中

	// マーカー演出用
	float markerAnimTimer_ = 0.0f;

private:

	void InitIntro();
	void UpdateIntro(float dt);
	void BeginPlay();

private:

	// アームの位置に描画するマーカーの更新
	void UpdateArmTargetMarker();

	// ミサイルの位置に描画するマーカーの更新
	void UpdateMissileTelegraphMarkers();

	// 銃の更新用
	void UpdateGun();

	// メテオ関連の制御（入力・ボスリクエスト・カメラ追従のON/OFF）
	void UpdateMeteorControl();

	// 
	void LineTarget();

	// 仮の床グリッド描画
	void AddFloorGrid();

	Vector3 CalcLookAtRotation(const Vector3& camPos, const Vector3& targetPos);

	// 演出更新
	void UpdateBossIntroGlint(float dt);

	// 演出開始
	void StartBossIntroGlint();
};