#pragma once
#include <memory>

#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Collision/ICollisionObject.h"
#include "Engine/Base/Collision/CollisionManager.h"
#include "Game/Entity/GameObject.h"

class Camera;
class Player;

class BossMissile : public GameObject, public ICollisionObject {

public:

	BossMissile() = default;
	~BossMissile() override = default;

	void Init(Camera* camera);

	void Update() override;

	void Draw() override;

	void Kill() override;

	void Spawn(const Vector3& startPos, const Vector3& direction, float speed);

	// 弧を描く用
	void SpawnCurve(
		const Vector3& startPos,
		const Vector3& controlPos,
		const Vector3& endPos,
		float duration
	);

	bool IsAlive() const { return isAlive_; }
	bool DidHitPlayer() const { return hitPlayer_; }

	void SetTranslate(const Vector3& t) { transform_.translate = t; }
	const Vector3& GetTranslate() const { return transform_.translate; }

	// ICollisionObject
	Vector3 GetCollisionPosition() const override;
	float GetCollisionRadius() const override;
	CollisionLayer GetCollisionLayer() const override { return CollisionLayer::EnemyMissile; }
	void OnCollision(ICollisionObject* other) override;

	CollisionManager* collisionManager_ = nullptr;
	bool collisionRegistered_ = false;
	void SetCollisionManager(CollisionManager* mgr) { collisionManager_ = mgr; }

private:

	Camera* camera_ = nullptr;
	std::unique_ptr<Object3d> object3d_ = nullptr;

	Transform transform_{};
	Vector3 direction_{ 0.0f, 0.0f, 1.0f };
	float speed_ = 0.0f;
	float radius_ = 0.7f;

	bool isAlive_ = false;
	bool hitPlayer_ = false;

	float lifeTimer_ = 0.0f;
	float maxLife_ = 6.0f;

	// =========================
	// 曲線移動用
	// =========================
	bool useCurve_ = false;
	Vector3 curveStart_{};
	Vector3 curveControl_{};
	Vector3 curveEnd_{};
	float curveT_ = 0.0f;
	float curveDuration_ = 1.2f;

	float lastCurveT_ = 0.0f;
	Vector3 lastCurvePos_{};

private:

	Vector3 EvalQuadraticBezier(float t) const;
};