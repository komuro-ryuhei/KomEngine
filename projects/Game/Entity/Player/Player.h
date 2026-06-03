#pragma once

// MyCalss
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/lib/Input/Input.h"
#include "struct.h"

#include "Engine/Base/2d/Sprite/Sprite.h"
#include "Game/Entity/Player/PlayerBullet.h"
#include "Engine/Base/Particle/ParticleEmitter.h"
#include "Engine/Base/Collision/ICollisionObject.h"
#include "Engine/Base/Collision/CollisionManager.h"
#include "Game/Entity/GameObject.h"

// C++
#include <algorithm>
#include <vector>

class Player : public GameObject, public ICollisionObject {

public:

	Player() = default;
	~Player();

	void Init(Camera* camera);

	void Update() override;
	void Draw() override;

	void Kill() override {
		isActive_ = false;
		canShoot_ = false;
		controlEnabled_ = false;
	}

	void ImGuiDebug();

public:

	// ----------------------- ICollisionObjectの実装 ----------------------- //
	Vector3 GetCollisionPosition() const override;
	float   GetCollisionRadius() const override;
	CollisionLayer GetCollisionLayer() const override;
	void OnCollision(ICollisionObject* other) override;

	void SetCollisionManager(CollisionManager* mgr) { collisionManager_ = mgr; }

public:

	float GetRadius() const;
	Transform GetTransform() const;
	Vector3 GetTranslate() const;
	std::vector<std::unique_ptr<PlayerBullet>>& GetBullets();
	int GetHP() const;
	bool GetInvincible() const;

	void SetInvincible(bool flag);
	void SetRotate(Vector3& rotate);
	void SetHeatGaugePos(const Vector2& pos) { heatGaugePos_ = pos; }

	bool IsInvincible() const;
	void Damage(int amount);
	bool IsLowHP(int hp) const;

	void SetCanShoot(bool can) { canShoot_ = can; }
	bool CanShoot() const { return canShoot_; }

	void SetGunMuzzlePos(const Vector3& pos) { gunMuzzlePos_ = pos; hasGunMuzzlePos_ = true; }

	bool IsAlive() const { return IsActive(); }

	bool IsCharging() const { return isCharging_; }
	float GetChargeRatio() const {
		if (chargeFullTime_ <= 0.0f) {
			return 0.0f;
		}
		return std::clamp(chargeTimer_ / chargeFullTime_, 0.0f, 1.0f);
	}

	// ----------------------- バリア ----------------------- //
	void SetBarrierActive(bool active) { isBarrierActive_ = active; }
	bool IsBarrierActive() const { return isBarrierActive_; }

	float GetBarrierRadius() const { return barrierRadius_; }
	Vector3 GetBarrierPosition() const { return GetTranslate(); }
	// ---------------------------------------------------- //

private:

	// 攻撃
	void Attack(float dt);

	// 弾の生成
	void SpawnBullet(int damage);

	// レティクルのスプライト更新
	void UpdateReticleSprite();

	// チャージ攻撃のエフェクト更新
	void ChargeEffect(float dt);

	// 手元の銃の更新
	void UpdateGun();

	// オーバーヒートゲージの更新
	void UpdateHeatGauge();

public:

	void RailMove();
	void RotateY90();

	// コントロール有効化・無効化
	void SetControlEnabled(bool enabled) { controlEnabled_ = enabled; }
	bool IsControlEnabled() const { return controlEnabled_; }

private:

	// カメラ
	Camera* camera_ = nullptr;
	// モデル
	std::unique_ptr<Object3d> object3d_ = nullptr;
	// 弾のリスト
	std::vector<std::unique_ptr<PlayerBullet>> bulletObjects_;

	// レティクルのスプライト
	std::unique_ptr<Sprite> reticleSprite_ = nullptr;

	// 当たり判定管理
	CollisionManager* collisionManager_ = nullptr;

	// SRT
	Transform transform_;

	// 速度
	float velocity_ = 0.05f;
	// 半径
	float radius_ = 1.0f;
	// HP
	int hp_ = 5;
	// 弾有効フラグ
	bool isBulletActive_ = false;

	// 無敵時間
	bool isInvincible_ = false;
	float invincibleTimer_ = 0.0f;

	// 連射制御
	float autofireInterval_ = 0.10f;
	float autofireTimer_ = 0.0f;

	// ----------------------- オーバーヒート & チャージ ----------------------- //
	float heat_ = 0.0f;
	float heatMax_ = 100.0f;
	float heatRecover_ = 30.0f; // ここまで冷えたら復帰（heatMax_の30%など）

	float heatCoolPerSec_ = 35.0f;      // 何もしてない時の冷却速度
	float heatCoolWhileCharge_ = 20.0f; // チャージ中の冷却速度

	float heatCostNormal_ = 12.0f;   // 通常弾1発の熱量
	float heatCostAutofire_ = 9.0f;  // 連射弾1発の熱量
	float heatCostCharged_ = 30.0f;  // フルチャージ弾の熱量
	bool  isOverheated_ = false;

	// チャージショット（Mouse0押し→離した瞬間に発射）
	bool  isCharging_ = false;
	float chargeTimer_ = 0.0f;
	float chargeMinTime_ = 0.25f;  // これ未満は「タップ＝通常弾」
	float chargeFullTime_ = 1.10f; // ここまで溜めたらフル扱い

	// マウスの前フレーム状態（Release検出用）
	bool prevMouse0Down_ = false;
	bool prevMouse1Down_ = false;

	// チャージの最大ダメージ（1〜この値まで増える）
	int chargeDamageMax_ = 5;

	// このフレームに弾を撃ったか
	bool firedThisFrame_ = false;

	// 射撃許可フラグ
	bool canShoot_ = true;

	// ----------------------- バリア ----------------------- //
	bool isBarrierActive_ = false;

	// 突進を防ぐ判定用の半径
	float barrierRadius_ = 3.5f;
	// ---------------------------------------------------- //

	// 銃の先端のワールド座標
	Vector3 gunMuzzlePos_{};
	bool hasGunMuzzlePos_ = false;

	std::unique_ptr<ParticleEmitter> muzzleEmitter_ = nullptr;
	// チャージ演出
	std::unique_ptr<ParticleEmitter> chargeCoreEmitter_ = nullptr;
	std::unique_ptr<ParticleEmitter> chargePulseEmitter_ = nullptr;
	std::unique_ptr<ParticleEmitter> chargeLineEmitter_ = nullptr;

	float chargeFxCoreTimer_ = 0.0f;
	float chargeFxPulseTimer_ = 0.0f;

	// コントロール有効フラグ
	bool controlEnabled_ = true;

	// 手元Gun
	std::unique_ptr<Object3d> gun_ = nullptr;

	// 手元Gunの見た目調整
	float gunDist_ = 2.0f;     // カメラ前方距離
	float gunRight_ = 0.45f;   // 右オフセット
	float gunDown_ = 0.35f;    // 下オフセット
	Vector3 gunScale_ = { 0.1f, 0.1f, 0.7f }; // 銃っぽい比率
	Vector3 gunRotate_ = { 0.0f, 0.0f, 0.0f }; // 銃っぽい比率
	Vector3 gunTranslate_ = { 0.2f, -0.2f, 1.5f }; // 銃っぽい比率
	Vector3 gunRotOffset_ = { 0.0f, 0.0f, 0.0f }; // 必要なら傾ける

	// ゲージ用スプライト
	std::unique_ptr<Sprite> heatGaugeBg_ = nullptr;
	std::unique_ptr<Sprite> heatGaugeFill_ = nullptr;
	float heatGaugeMaxWidth_ = 256.0f;
	float heatGaugeHeight_ = 32.0f;
	Vector2 heatGaugePos_{ 40.0f, 620.0f };
};