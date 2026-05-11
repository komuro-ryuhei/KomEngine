#pragma once
#include "Engine/Base/Collision/ICollisionObject.h"
#include "Engine/lib/Math/MyMath.h"
#include <memory>

class BossRetreatAttack : public ICollisionObject {

public:

	struct Params {
		float shrinkTime = 0.20f;
		float moveTime = 0.25f;
		float unflattenTime = 0.25f;
		float flattenTime = 0.20f;
		float growTime = 0.20f;

		float backZOffset = 30.0f;
		float upOffset = 8.0f;

		float minScaleXZ = 0.001f;
		float minScaleY = 0.9f;
	};

	BossRetreatAttack();
	~BossRetreatAttack() override;

	void Init();

	void Update(float dt);

	void Start(const Vector3& startPos, const Vector3& baseBodyScale, const Vector3& baseArmScale);
	void ForceEnd();
	void RequestReturn();

	bool IsActive() const { return state_ != nullptr; }
	bool IsHolding() const;
	bool ConsumeHoldEntered();
	bool ConsumeFinished();

	const Vector3& GetTranslate() const { return translate_; }
	const Vector3& GetBodyScale() const { return bodyScale_; }
	const Vector3& GetArmScale() const { return armScale_; }
	const Vector3& GetBackPos() const { return backPos_; }
	bool IsInvulnerable() const { return invulnerable_; }

	Params& GetParams() { return params_; }
	const Params& GetParams() const { return params_; }

	// ICollisionObject
	Vector3 GetCollisionPosition() const override { return translate_; }
	float GetCollisionRadius() const override { return 0.0f; }
	CollisionLayer GetCollisionLayer() const override { return CollisionLayer::Enemy; }
	void OnCollision(ICollisionObject* other) override { other; }

private:

	class IRetreatState;
	class MoveOutState;
	class StayUnflattenState;
	class StayHoldState;
	class ReturnState;

	void ChangeState(std::unique_ptr<IRetreatState> nextState);
	void FinishReturn();
	void UpdateMoveOut(float dt);
	void UpdateStayUnflatten(float dt);
	void UpdateStayHold(float dt);
	void UpdateReturn(float dt);

	void ApplyScaleFactor(float factorXZ, float factorY);

private:

	Params params_{};

	std::unique_ptr<IRetreatState> state_;

	float timer_ = 0.0f;

	Vector3 startPos_{};
	Vector3 backPos_{};

	Vector3 baseBodyScale_{ 2.0f, 2.0f, 2.0f };
	Vector3 baseArmScale_{ 1.0f, 1.0f, 1.0f };

	Vector3 translate_{};
	Vector3 bodyScale_{ 2.0f, 2.0f, 2.0f };
	Vector3 armScale_{ 1.0f, 1.0f, 1.0f };

	bool invulnerable_ = false;
	bool requestReturn_ = false;
	bool holdEntered_ = false;
	bool finished_ = false;
};