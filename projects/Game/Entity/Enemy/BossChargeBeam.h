#pragma once
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Collision/ICollisionObject.h"

class Camera;

class BossChargeBeam : public ICollisionObject {

public:

	void Init(Camera* camera);
	void Update(float dt);
	void Draw();

	void Fire(const Vector3& startPos, const Vector3& dir);
	void Destroy();

	bool IsActive() const { return active_; }
	bool DidHitPlayer() const { return hitPlayer_; }

	// ICollisionObject
	Vector3 GetCollisionPosition() const override { return position_; }
	float GetCollisionRadius() const override { return active_ ? radius_ : 0.0f; }
	CollisionLayer GetCollisionLayer() const override { return CollisionLayer::EnemyBullet; }
	void OnCollision(ICollisionObject* other) override;

private:

	Camera* camera_ = nullptr;
	std::unique_ptr<Object3d> obj_;

	bool active_ = false;
	bool hitPlayer_ = false;

	Vector3 position_{};
	Vector3 direction_{ 0.0f, 0.0f, 1.0f };
	float speed_ = 1.0f;
	float radius_ = 1.1f;

	float life_ = 0.0f;
	float minLife_ = 0.4f;
	float maxLife_ = 3.0f;

	Vector3 startScale_{ 0.6f, 0.6f, 9.0f };
	Vector3 endScale_{ 0.08f, 0.08f, 2.0f };
};