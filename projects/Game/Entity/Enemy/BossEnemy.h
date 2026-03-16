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

#include <vector>
#include <array>
#include <memory>
#include <algorithm>

class Player;
class Camera;

class BossEnemy : public ICollisionObject {

private:

	enum class AttackPhase {
		None,
		SingleLeft,
		SingleRight,
		BothHands,
		WaitMeteor,
	};

	enum class EnrageTransitionPhase {
		None,
		Knockback,
		Wait,
		Recover,
	};

	AttackPhase attackPhase_ = AttackPhase::SingleLeft;
	bool meteorRequest_ = false; // 両手攻撃完了後にtrue

	// HPエフェクト用チップ
	struct HpChip {
		std::unique_ptr<Sprite> sprite;
		Vector2 pos;    // 画面上の位置
		Vector2 vel;    // 速度（ピクセル/秒）
		float   life = 0.0f; // 残り寿命（秒）
	};

	std::vector<HpChip> hpChips_;

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

	void Init(Camera* camera);

	void Update();

	void Draw();

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

	// 被弾フラッシュ開始（部位別）
	void StartBodyHitFlash() { bodyHitFlashTime_ = hitFlashDuration_; }
	void StartLeftHitFlash() { leftHitFlashTime_ = hitFlashDuration_; }
	void StartRightHitFlash() { rightHitFlashTime_ = hitFlashDuration_; }

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
	Vector3 GetTranslate() const;
	bool GetIsmoveRight() const { return isMoveRight_; }

	void SetRotate(const Vector3& rotate);
	void SetTranslate(Vector3 translate);

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
	bool   HasLanded() const { return hasLanded_; }

	// ----- 攻撃 ----- //
	// 攻撃中かどうか（腕が伸びているフェーズか）を外からチェック用
	bool IsExtending() const { return isExtending_; }

	// 現在攻撃に使っている腕が左かどうか
	bool IsLeftArmAttacking() const { return attackPhase_ == AttackPhase::SingleLeft; }

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

	// AttackPhase 取得系（UpdateArmTargetMarker用）
	bool IsRightArmAttacking() const { return attackPhase_ == AttackPhase::SingleRight; }
	bool IsBothHandsAttacking() const { return attackPhase_ == AttackPhase::BothHands; }

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
	bool IsEnrageTransitioning() const { return enrageTransitioning_; }
	void StartEnrageTransition(float duration);
	void UpdateEnrageTransition(float dt);

	// 攻撃を即中断
	void CancelAllAttacks();

private:

	void Attack();
	void Move();
	void TitleSceneMove();

	// 減ったぶんからチップを生成
	void SpawnHpChips(float prevWidth, float newWidth);

	// 
	void DamageShake();

public:
	void AddHitToAttackingArm();
	void InitTitleScenePos();

	// 流星攻撃との連携
	bool ConsumeMeteorRequest(); // trueを返したタイミングでフラグを消費
	void OnMeteorFinished();     // メテオ終了後に呼ぶ

	// 流星攻撃を開始すべきか
	bool ShouldStartMeteor() const;

	// チャージビーム要求
	void RequestChargeAttack(bool targetLeft);
	bool ConsumeChargeRequest();

	// 
	bool pendingChargeAfterRetreat_ = false;
	bool pendingMeteorAfterCharge_ = false;
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


	// ----------------------- Armor（周回装甲） ----------------------- //
	struct ArmorUnit {
		std::unique_ptr<Object3d> obj;
		bool alive = true;   // 破壊済みならfalse
		float angle = 0.0f;  // 周回角度（ラジアン）
	};

	std::vector<ArmorUnit> armors_;
	int   armorInitialCount_ = 12;
	float armorOrbitRadius_ = 3.0f; // 本体中心からの半径
	float armorOrbitSpeed_ = 0.9f;  // rad/s
	float armorFloatAmp_ = 0.18f;   // 上下振幅
	float armorFloatSpeed_ = 1.6f;  // 上下速度
	float armorTime_ = 0.0f;
	float armorGlobalAngle_ = 0.0f; // 生存アーマーを等分配置するための全体回転角
	Vector3 armorScale_ = { 0.3f, 0.3f, 0.3f };
	bool armorRebuildRequest_ = false;

	void InitArmors();
	void UpdateArmors(float dt);
	void DrawArmors();
	void BreakOneArmor();
	int  GetAliveArmorCount() const;

	// 腕は腕攻撃時のみ表示（描画・当たり判定を無効化するため）
	bool leftArmVisible_ = false;
	bool rightArmVisible_ = false;


	// 
	std::unique_ptr<Sprite> hpSprite_;

	// 攻撃用のタイマーと状態
	float attackTimer_ = 0.0f;
	float attackInterval_ = 2.0f; // 2秒周期
	bool isExtending_ = true;     // 腕を伸ばしているか
	float attackCooldown_ = 0.0f;
	bool isAttacking_ = false;
	bool attackLeftArm_ = true;
	float attackSpeed_ = 0.1f; // 腕の伸縮速度
	int leftArmHitCount_ = 0;
	int rightArmHitCount_ = 0;
	const int maxHitCount_ = 5;

	// 戻り速度（今までの 0.5f / 0.6f を変数化）
	float armReturnSpeedSingle_ = 0.5f;
	float armReturnSpeedBoth_ = 0.6f;

	// 通常値（怒り解除しないなら保存目的は「倍率適用の基準」）
	float baseAttackSpeed_ = 0.1f;
	float baseArmReturnSpeedSingle_ = 0.5f;
	float baseArmReturnSpeedBoth_ = 0.6f;

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

	// 撃破後の墜落制御
	bool  fallStarted_ = false;   // 落下開始したか
	bool  hasLanded_ = false;   // 地面に着いたか
	float fallVelY_ = 0.0f;    // 落下速度
	float gravityY_ = -0.006f;  // 重力加速度（毎フレーム加算）
	float groundY_ = -5.0f;   // 地面のY（glassObject_ と合わせた）
	// 回転しながら落下用
	float fallRotateStart_ = 0.0f;    // 開始角度（今の回転を保存）
	float fallRotateEnd_ = -1.2f;   // 最終角度（ラジアン）＝約 -70 度前に倒す

	// 落下中シェイク用
	float fallShakeTime_ = 0.0f;
	float fallShakeAmplitude_ = 0.25f; // 揺れ幅（XZ方向）

	// 落下後のカメラシェイクフラグ
	bool landingShakeDone_ = false;

	// 被弾時のシェイク
	float bodyHitShakeTime_ = 0.0f;
	float leftHitShakeTime_ = 0.0f;
	float rightHitShakeTime_ = 0.0f;

	float hitShakeDuration_ = 0.15f;  // 揺れる時間(秒)
	float hitShakeAmplitude_ = 0.25f;  // 揺れ幅

	// 被弾時のフラッシュ（部位ごと）
	float bodyHitFlashTime_ = 0.0f;
	float leftHitFlashTime_ = 0.0f;
	float rightHitFlashTime_ = 0.0f;

	float hitFlashDuration_ = 0.1f; // フラッシュ時間

	// 戦闘有効化・無効化
	bool combatEnabled_ = true;

	// -------------------- 退避攻撃（奥へ行く攻撃） -------------------- // 
	enum class RetreatPhase { None, MoveOut, Stay, Return };
	RetreatPhase retreatPhase_ = RetreatPhase::None;

	bool invulnerable_ = false;

	float retreatT_ = 0.0f;
	float retreatOutTime_ = 0.35f;
	float retreatStayTime_ = 1.20f;
	float retreatReturnTime_ = 0.40f;

	float retreatBackZOffset_ = 30.0f;
	float retreatUpOffset_ = 8.0f;

	float retreatMinScaleFactor_ = 0.15f;

	float retreatShrinkTime_ = 0.20f;  // 縮むだけの時間
	float retreatMoveTime_ = 0.25f;  // 移動だけの時間（奥へ/戻り共通にしてもOK）
	float retreatGrowTime_ = 0.20f;  // 戻すだけの時間
	float retreatFlattenTime_ = 0.20f; // 戻る前にペラ化する時間

	float retreatMinScaleXZ_ = 0.0f;  // 横(XZ)の最小倍率
	float retreatMinScaleY_ = 0.9f;  // 縦(Y)はあまり変えない（0.85〜1.0推奨）


	Vector3 retreatStartPos_{};
	Vector3 retreatBackPos_{};

	Vector3 baseBodyScale_{ 2.0f,2.0f,2.0f };
	Vector3 baseArmScale_{ 1.0f,1.0f,1.0f };

	// Stay中の段階
	enum class RetreatStayPhase { Unflatten, Hold };
	RetreatStayPhase retreatStayPhase_ = RetreatStayPhase::Unflatten;

	// 奥でペラ→通常へ戻す時間
	float retreatUnflattenTime_ = 0.25f;

	// 通常状態で奥に留まる時間（数秒）
	float retreatHoldTime_ = 2.0f;   // 好きな秒数にしてOK

	bool missileStartedThisRetreat_ = false;

	struct MissileSlot {
		std::unique_ptr<Object3d> obj;
		std::unique_ptr<EnemyBullet> bullet;
		bool launched = false;
	};

	enum class MissilePhase { None, Telegraph, Launch };

	std::array<MissileSlot, 4> missiles_;
	MissilePhase missilePhase_ = MissilePhase::None;
	float missileT_ = 0.0f;

	float missileTelegraphTime_ = 1.0f; // 予告表示
	float missileRadius_ = 2.5f;        // 半円の半径
	float missileHeight_ = 2.0f;        // ボス上方向オフセット
	float missileSpeed_ = 0.2f;         // 発射速度

	bool missileHitPlayer_ = false; // 当たったらtrue
	float missileHitDist_ = 0.7f;   // 当たり判定距離
	float missileMaxDist_ = 200.0f; // 遠すぎたら消す

	float missileLaunchTimeout_ = 6.0f;

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

	// チャージ中のエフェクト
	float chargeFxRingTimer_ = 0.0f;
	float chargeFxCylinderTimer_ = 0.0f;
	float chargeFxRibbonTimer_ = 0.0f;
	Vector3 chargeFxOffset_{ 0.0f, 2.0f, 0.0f }; // 胴体中心の少し上あたり

	float chargeFxCoreTimer_ = 0.0f;
	float chargeFxPulseTimer_ = 0.0f;

	// 怒り遷移演出用
	bool enrageTransitioning_ = false;
	EnrageTransitionPhase enragePhase_ = EnrageTransitionPhase::None;

	float enrageTransitionTimer_ = 0.0f;
	float enrageTransitionDuration_ = 0.0f;

	// 各フェーズ時間
	float enrageKnockbackDuration_ = 0.25f;
	float enrageWaitDuration_ = 1.50f;
	float enrageRecoverDuration_ = 0.35f;

	// 移動用
	Vector3 enrageStartPos_{};
	Vector3 enrageKnockbackPos_{};

	// ノックバック量
	float enrageKnockbackDistance_ = 3.5f;
	float enrageKnockbackLift_ = 1.0f;

	// シェイク
	float enrageShakeAmplitude_ = 0.12f;
	float enrageShakeFrequency_ = 40.0f;

	// 赤フラッシュ
	float enrageFlashSpeed_ = 10.0f;

	// 復帰時衝撃波
	bool enrageShockwaveEmitted_ = false;

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

	Vector3 chargeCoreOffset_{ 0.0f, 3.0f, 0.0f };
	int chargeCoreHp_ = 12;

private:

	void UpdateRetreat(float dt);

	// ミサイル攻撃関数
	void StartMissileVolley();          // 予告開始
	void UpdateMissileVolley(float dt); // 毎フレーム更新
	void DrawMissileVolley();           // 描画

	// チャージビーム
	void UpdateChargeBeamShot(float dt);
	void UpdateChargeCrossPose(float dt);

	// チャージ時のエフェクト
	void ChargeEffect(float dt);
};