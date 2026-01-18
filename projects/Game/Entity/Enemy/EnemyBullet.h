#pragma once

// MyCalss
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/lib/Input/Input.h"
#include "struct.h"

#include "ICollisionObject.h"

class EnemyBullet : public ICollisionObject {

public:

	void Init(Camera* camera, Object3d* object3d);

	void Update();

	void Draw();

	void ImGuiDebug();

	float GetRadius() const;
	Vector3 GetTranslate() const;
	void SetTranlate(Vector3 translate);
	void SetDirection(const Vector3& direction);
	void SetDestroyOnPlayerHit(bool enable) { destroyOnPlayerHit_ = enable; }

	void SetSpeed(float speed);

	bool IsDead() const { return isDead_; }
	bool DidHitPlayer() const { return hitPlayer_; }
	void Kill() { isDead_ = true; }

	// ===== ICollisionObject =====
	Vector3 GetCollisionPosition() const override { return transform_.translate; }
	float GetCollisionRadius()   const override { return radius_; }
	CollisionLayer GetCollisionLayer() const override { return CollisionLayer::EnemyBullet; }
	void OnCollision(ICollisionObject* other) override;

private:
	Camera* camera_ = nullptr;

	Object3d* object3d_ = nullptr;

	Transform transform_;

	float speed_ = 0.003f;
	Vector3 direction_;

	float radius_ = 0.08f;

	bool isDead_ = false;
	bool hitPlayer_ = false;

	// プレイヤーに当たったら消えるかどうか
	bool destroyOnPlayerHit_ = true;
};