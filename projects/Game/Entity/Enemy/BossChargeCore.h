#pragma once
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Collision/ICollisionObject.h"

class Camera;
class PlayerBullet;

class BossChargeCore : public ICollisionObject {

public:

	void Init(Camera* camera);
	void Update(float dt);
	void Draw();

	void Activate(const Vector3& worldPos);
	void Deactivate();

	bool IsActive() const { return active_; }
	bool IsBroken() const { return hp_ <= 0; }

	void SetWorldPos(const Vector3& worldPos);
	Vector3 GetWorldPos() const { return worldPos_; }

	void ResetHP(int hp) { hp_ = hp; }
	int GetHP() const { return hp_; }

	// ICollisionObject
	Vector3 GetCollisionPosition() const override { return worldPos_; }
	float GetCollisionRadius() const override { return active_ ? radius_ : 0.0f; }
	CollisionLayer GetCollisionLayer() const override { return CollisionLayer::EnemyCore; }
	void OnCollision(ICollisionObject* other) override;

private:

	Camera* camera_ = nullptr;
	std::unique_ptr<Object3d> obj_;

	bool active_ = false;
	int hp_ = 8;
	int maxHp_ = 8;

	float radius_ = 1.2f;
	Vector3 worldPos_{};
	Vector3 scale_{ 1.2f, 1.2f, 1.2f };

	float rotY_ = 0.0f;
};