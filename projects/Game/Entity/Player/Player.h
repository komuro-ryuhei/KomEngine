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
#include "ICollisionObject.h"
#include "CollisionManager.h"

// C++
#include <algorithm>
#include <vector>

class Player : public ICollisionObject {

public:

	Player() = default;
	~Player();

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

	bool IsInvincible() const;
	void Damage(int amount);
	bool IsLowHP(int hp) const;

	void SetCanShoot(bool can) { canShoot_ = can; }
	bool CanShoot() const { return canShoot_; }

	void SetGunMuzzlePos(const Vector3& pos) { gunMuzzlePos_ = pos; hasGunMuzzlePos_ = true; }

private:
	void Attack();

	void SpawnBullet();

	void UpdateReticleSprite();

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
	int hp_ = 50;
	// 弾有効フラグ
	bool isBulletActive_ = false;

	// 無敵時間
	bool isInvincible_ = false;
	float invincibleTimer_ = 0.0f;

	// 連射制御（左クリック長押し用）
	float autofireInterval_ = 0.10f;
	float autofireTimer_ = 0.0f;

	// 射撃許可フラグ（初期は撃てる）
	bool canShoot_ = true;

	// 銃の先端のワールド座標
	Vector3 gunMuzzlePos_{};
	bool    hasGunMuzzlePos_ = false;

	std::unique_ptr<ParticleEmitter> muzzleEmitter_ = nullptr;

	// コントロール有効フラグ
	bool controlEnabled_ = true;
};