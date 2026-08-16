#pragma once

// Engine
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/2d/Sprite/Sprite.h"
#include "Engine/Base/Collision/ICollisionObject.h"
#include "Engine/Base/Collision/CollisionManager.h"

// Game
#include "Game/Entity/Enemy/EnemyBullet.h"
#include "Game/Entity/Enemy/BossChargeCore.h"
#include "Game/Entity/Enemy/BossChargeBeam.h"
#include "Game/Entity/Enemy/BossDeathController.h"
#include "Game/Entity/Enemy/BossArmorController.h"
#include "Game/Entity/Enemy/BossDizzyStarController.h"
#include "Game/Entity/Enemy/BossChargeEffectController.h"
#include "Game/Entity/Enemy/BossEnrageTransitionController.h"
#include "Game/Gameplay/Enemy/BossArmAttackController.h"
#include "Game/Entity/GameObject.h"
#include "Game/UI/BossHpUI.h"

#include <vector>
#include <array>
#include <memory>
#include <algorithm>

class Player;
class Camera;

class BossEnemy : public GameObject, public ICollisionObject {

	friend class BossArmAttackController;

private:

	enum class AttackPhase {
		None,
		SingleLeft,
		SingleRight,
		BothHands,
		WaitMeteor,
	};

	// 腕攻撃用State
	class ArmAttackState {

	public:

		virtual ~ArmAttackState() = default;

		virtual void Enter(BossEnemy& boss) {
			(void)boss;
		}

		virtual void Update(BossEnemy& boss, float dt) = 0;

		virtual AttackPhase GetPhase() const = 0;
	};

	class SingleArmAttackState;
	class BothHandsAttackState;
	class WaitMeteorAttackState;

	AttackPhase attackPhase_ = AttackPhase::None;

	// 現在の腕攻撃State
	std::unique_ptr<ArmAttackState> armAttackState_ = nullptr;

	struct PartCollider : public ICollisionObject
	{
		BossEnemy* owner = nullptr;
		enum class Part { Body, LeftArm, RightArm } part;

		Vector3 GetCollisionPosition() const override;
		float   GetCollisionRadius() const override;
		CollisionLayer GetCollisionLayer() const override { return CollisionLayer::Enemy; }
		void OnCollision(ICollisionObject* other) override;
	};

	PartCollider bodyCol_;
	PartCollider leftCol_;
	PartCollider rightCol_;

public:

	~BossEnemy();

	void Init(Camera* camera);

	void Update() override;
	void Draw() override;

	void Kill() override {
		isActive_ = false;
		combatEnabled_ = false;
		isAttack_ = false;
	}

	void ImGuiDebug();

public:

	// ----------------------- ICollisionObjectの実装 ----------------------- //
	Vector3 GetCollisionPosition() const override;
	float   GetCollisionRadius() const override;
	CollisionLayer GetCollisionLayer() const override;
	void OnCollision(ICollisionObject* other) override;

	// 被弾シェイク開始（部位別）
	void StartBodyHitShake() { bodyHitShakeTime_ = hitShakeDuration_; }
	void StartLeftArmHitShake() { leftHitShakeTime_ = hitShakeDuration_; }
	void StartRightArmHitShake() { rightHitShakeTime_ = hitShakeDuration_; }

	// チャージビーム（発射物）
	void StartChargeBeamShot(bool useLeftArm);
	bool IsChargeBeamShotActive() const;

	// 怒り状態かどうか
	bool IsEnraged() const { return isEnraged_; }
	void SetEnraged(bool enraged);
	int GetMaxHp() const { return maxHp_; }

public:

	// getter・setter
	float GetRadius() const;
	Transform GetTransform() const;
	Vector3 GetRotate() const { return transform_.rotate; }
	Vector3 GetTranslate() const;
	bool GetIsmoveRight() const { return isMoveRight_; }

	void SetRotate(const Vector3& rotate);
	void SetTranslate(Vector3 translate);

	void SetDizzyEffectActive(bool active);
	bool IsDizzyEffectActive() const {
		return dizzyStarController_ && dizzyStarController_->IsActive();
	}

	void SetPlayer(Player* player) { player_ = player; }

	void SetAttack(bool isAttack) { isAttack_ = isAttack; }
	void SetAttackSpeed(float speed) { attackSpeed_ = speed; }
	float GetAttackSpeed() const { return attackSpeed_; }

	void SetInTitleScene(bool isTitleScene) { isInTitleScene_ = isTitleScene; }

	void SetHP(int hp) { hp_ = hp; }

	// 当たり判定管理
	void SetCollisionManager(CollisionManager* mgr) {

		collisionManager_ = mgr;

		if (collisionManager_) {
			collisionManager_->Register(&bodyCol_);
			collisionManager_->Register(&leftCol_);
			collisionManager_->Register(&rightCol_);

			if (chargeCore_) {
				collisionManager_->Register(chargeCore_.get());
			}
			if (chargeBeam_) {
				collisionManager_->Register(chargeBeam_.get());
			}
		}
	}

	// ----- 部位 ----- //
	Object3d* GetBody() const { return object3d_.get(); }
	Object3d* GetLeftArm() const { return leftArm_.get(); }
	Object3d* GetRightArm() const { return rightArm_.get(); }

	// ----- 右手 ----- //
	void    SetRightHandScale(const Vector3& s);
	Vector3 GetRightHandWorldPos() const;
	float   GetRightHandRadius() const;

	// ----- 左手 ----- //
	void    SetLeftHandScale(const Vector3& s);
	Vector3 GetLeftHandWorldPos() const;
	float   GetLeftHandRadius() const;

	// HP
	void   Damage(int v);
	int    GetHP() const { return hp_; }
	bool   IsDead() const { return hp_ <= 0; }

	// 着地(墜落)したかのフラグ
	bool   HasLanded() const {
		return deathController_ && deathController_->HasLanded();
	}

	// 攻撃中かどうか（腕が伸びているフェーズか）を外からチェック用
	bool IsExtending() const {
		return armAttackController_
			? armAttackController_->IsExtending()
			: false;
	}

	// 今攻撃に使っている腕のワールド座標
	Vector3 GetCurrentArmWorldPos() const;

	// 腕のヒット数
	int GetLeftHitCount() const { return leftArmHitCount_; }
	int GetRightHitCount() const { return rightArmHitCount_; }

	// 腕が破壊されているかどうか
	bool IsLeftArmBroken() const { return leftArmHitCount_ >= maxHitCount_; }
	bool IsRightArmBroken() const { return rightArmHitCount_ >= maxHitCount_; }

public:

	void AddHitLeftArm() { ++leftArmHitCount_; }
	void AddHitRightArm() { ++rightArmHitCount_; }


	bool IsLeftArmAttacking() const {
		return armComboActive_ && attackPhase_ == AttackPhase::SingleLeft;
	}

	bool IsRightArmAttacking() const {
		return armComboActive_ && attackPhase_ == AttackPhase::SingleRight;
	}

	bool IsBothHandsAttacking() const {
		return armComboActive_ && attackPhase_ == AttackPhase::BothHands;
	}

	// 最大ヒット数（ターゲット消す条件に使う）
	int GetMaxHitCount() const { return maxHitCount_; }

	// 退避→奥から攻撃→復帰（退避中は無敵）
	void StartRetreatAttack();
	bool IsInvulnerable() const { return invulnerable_; }

	// 腕コンボ（右→左→両手）の開始/完了
	void StartArmCombo();
	bool ConsumeArmComboFinished();
	bool IsArmComboActive() const { return armComboActive_; }

	void CancelAttacksForMeteor();

	// 怒り遷移中
	bool IsEnrageTransitioning() const {
		return enrageController_ && enrageController_->IsActive();
	}
	void StartEnrageTransition(float duration);

	// 攻撃を即中断
	void CancelAllAttacks();

private:

	// 攻撃
	void Attack();
	bool CanUpdateAttack() const;
	void UpdateSingleArmAttack(float dt);
	void UpdateBothHandsAttack(float dt);
	void UpdateWaitMeteorAttack();

	// 腕攻撃Stateの切り替え
	void ChangeArmAttackState(std::unique_ptr<ArmAttackState> nextState);
	void StartSingleArmPhase(AttackPhase phase);
	void StartBothHandsPhase();
	void FinishArmCombo();
	void ResetArmAttackState();

	// 動き
	void Move();
	// タイトルシーンでの動き
	void TitleSceneMove();

	// Update処理の分割
	void UpdateCommonEffects(float dt);
	void UpdateBossObjects(float dt);
	void UpdateDamageTimers(float dt);
	void UpdateHpUI(float dt);
	void UpdateDead(float dt);
	void UpdateAlive(float dt);
	void UpdateArmVisibility();
	void UpdateVisualScaleAndCollisionRadius();

	// 
	void DamageShake();

public:
	void AddHitToAttackingArm();
	void InitTitleScenePos();

	// 流星攻撃との連携
	void OnMeteorFinished();     // メテオ終了後に呼ぶ

	// チャージビーム要求
	void RequestChargeAttack(bool targetLeft);
	bool ConsumeChargeRequest();

	// 
	bool nextChargeTargetLeft_ = true; // 次回チャージで狙う腕


	// チャージ中か
	bool IsChargeActive() const { return chargeActive_; }
	bool IsChargeTargetLeft() const { return chargeTargetLeft_; }
	void SetChargeActive(bool a) { chargeActive_ = a; }
	void SetChargeTargetLeft(bool l) { chargeTargetLeft_ = l; }

	// チャージ攻撃終了通知
	void OnChargeAttackFinished();

	void HPDraw();

	// 戦闘有効化・無効化
	void SetCombatEnabled(bool enabled) { combatEnabled_ = enabled; }
	bool IsCombatEnabled() const { return combatEnabled_; }

	// 退避攻撃関連
	bool ConsumeRetreatRequest();
	void SetInvulnerable(bool v) { invulnerable_ = v; }

	void SetRetreating(bool v) { retreatActive_ = v; }
	bool IsRetreating() const { return retreatActive_; }

	void SetCameraFocusPos(const Vector3& p) { retreatCameraFocusPos_ = p; }
	Vector3 GetCameraFocusPos() const { return retreatCameraFocusPos_; }
	bool WantsCameraFocus() const { return retreatActive_; }

	const Vector3& GetBaseBodyScale() const { return baseBodyScale_; }
	const Vector3& GetBaseArmScale() const { return baseArmScale_; }

	void ApplyRetreatPose(const Vector3& pos, const Vector3& bodyScale, const Vector3& armScale);
	void ClearRetreatVisualOverride();

	// チャージコア
	void ActivateChargeCore();
	void DeactivateChargeCore();
	bool IsChargeCoreBroken() const;
	bool IsChargeCoreActive() const;
	Vector3 GetChargeCoreWorldPos() const;

private:
	// カメラ
	Camera* camera_ = nullptr;
	// SRT
	Transform transform_;

	// 当たり判定管理
	CollisionManager* collisionManager_ = nullptr;

	// 半径
	float bodyRadius_ = 2.2f; // 胴体用
	float leftArmRadius_ = 1.0f; // 左腕用
	float rightArmRadius_ = 1.0f; // 右腕用

	Player* player_ = nullptr;

	// モデル
	std::unique_ptr<Object3d> object3d_ = nullptr;
	// 部位
	std::unique_ptr<Object3d> leftArm_;
	std::unique_ptr<Object3d> rightArm_;

	// HP
	std::unique_ptr<BossHpUI> hpUI_ = nullptr;

	// 撃破演出
	std::unique_ptr<BossDeathController> deathController_ = nullptr;

	// 装甲管理
	std::unique_ptr<BossArmorController> armorController_ = nullptr;

	// スタン中の星演出
	std::unique_ptr<BossDizzyStarController> dizzyStarController_ = nullptr;

	// 腕は腕攻撃時のみ表示（描画・当たり判定を無効化するため）
	bool leftArmVisible_ = false;
	bool rightArmVisible_ = false;

	// 攻撃用のタイマーと状態
	float attackTimer_ = 0.0f;
	float attackInterval_ = 2.0f; // 2秒周期
	float attackCooldown_ = 0.0f;
	bool isAttacking_ = false;
	bool attackLeftArm_ = true;
	float attackSpeed_ = 0.1f; // 腕の伸縮速度
	int leftArmHitCount_ = 0;
	int rightArmHitCount_ = 0;
	const int maxHitCount_ = 5;

	Vector3 rightBothTelegraphTargetPos_{};

	// 通常値
	float baseAttackSpeed_ = 0.1f;

	// 怒り倍率
	float enragedArmSpeedMul_ = 1.6f;

	// 攻撃用フラグ
	bool isAttack_ = true;

	// TitleScene用のフラグ
	bool isInTitleScene_ = false;

	// 
	Vector3 rightArmPos_;
	Vector3 leftArmPos_;
	Vector3 rightArmRot_;
	Vector3 leftArmRot_;

	bool armComboActive_ = false;
	bool armComboFinished_ = false;

	bool isMoveRight_ = false;
	bool pushEnter_ = true;

	// HP
	int maxHp_ = 20;
	int hp_ = maxHp_;

	// 両手攻撃用：左右個別に伸縮管理
	bool leftExtending_ = true;
	bool rightExtending_ = true;

	// 被弾時のシェイク
	float bodyHitShakeTime_ = 0.0f;
	float leftHitShakeTime_ = 0.0f;
	float rightHitShakeTime_ = 0.0f;

	float hitShakeDuration_ = 0.15f;  // 揺れる時間(秒)
	float hitShakeAmplitude_ = 0.25f;  // 揺れ幅

	// 戦闘有効化・無効化
	bool combatEnabled_ = true;

	bool invulnerable_ = false;

	Vector3 baseBodyScale_{ 2.0f,2.0f,2.0f };
	Vector3 baseArmScale_{ 1.0f,1.0f,1.0f };

	// チャージビーム発射物
	struct ChargeBeamShot {
		std::unique_ptr<Object3d> obj;
		std::unique_ptr<EnemyBullet> bullet;
	};

	ChargeBeamShot chargeShot_{};
	float chargeShotLife_ = 0.0f;
	float chargeShotMaxLife_ = 3.0f;
	float chargeShotMinLife_ = 3.0f;

	// チャージビームのスケール(開始前と開始後)
	Vector3 chargeBeamStartScale_{ 0.5f, 0.5f, 10.0f };
	Vector3 chargeBeamEndScale_{ 0.05f, 0.05f, 2.0f };

	// チャージ攻撃要求
	bool chargeRequest_ = false;
	bool chargeActive_ = false;
	bool chargeTargetLeft_ = false;

	// チャージ中の「腕クロス」演出
	bool chargePoseSaved_ = false;
	Vector3 chargeSavedLeftArmPos_{};
	Vector3 chargeSavedRightArmPos_{};
	float chargePoseLerp_ = 0.0f;
	float chargePoseInSpeed_ = 6.0f;    // 入り（大きいほど速い）
	float chargePoseOutSpeed_ = 8.0f;   // 戻り
	float chargeCrossZOffset_ = 2.5f;;  // 腕の重なり用Zずらし

	// 怒りモード
	bool isEnraged_ = false;

	bool chargeShotHitOnce_ = false;

	// 
	bool retreatRequest_ = false;
	bool retreatActive_ = false;
	Vector3 retreatCameraFocusPos_{};

	bool retreatVisualOverride_ = false;
	Vector3 retreatBodyScale_{ 2.0f, 2.0f, 2.0f };
	Vector3 retreatArmScale_{ 1.0f, 1.0f, 1.0f };

	// チャージコア、チャージビーム
	std::unique_ptr<BossChargeCore> chargeCore_;
	std::unique_ptr<BossChargeBeam> chargeBeam_;

	// チャージ中のパーティクル演出
	std::unique_ptr<BossChargeEffectController> chargeEffectController_ = nullptr;

	// 怒り遷移演出
	std::unique_ptr<BossEnrageTransitionController> enrageController_ = nullptr;

	// 腕攻撃の挙動管理
	std::unique_ptr<BossArmAttackController>armAttackController_ = nullptr;

	Vector3 chargeCoreOffset_{ 0.0f, 4.2f, 0.0f };
	int chargeCoreHp_ = 12;

	bool coreBreakReactionActive_ = false;
	float coreBreakReactionTimer_ = 0.0f;
	float coreBreakKnockbackTime_ = 0.20f;
	Vector3 coreBreakKnockbackStart_{};
	Vector3 coreBreakKnockbackEnd_{};

	bool coreBreakEffectPlayed_ = false;

private:

	// チャージビーム
	void UpdateChargeBeamShot(float dt);
	void UpdateChargeCrossPose(float dt);

	void StartCoreBreakReaction();
	void UpdateCoreBreakReaction(float dt);
};