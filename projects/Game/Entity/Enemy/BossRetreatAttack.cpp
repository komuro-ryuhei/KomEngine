#include "BossRetreatAttack.h"

void BossRetreatAttack::Init() {

	phase_ = Phase::None;
	timer_ = 0.0f;
	invulnerable_ = false;
	requestReturn_ = false;
	holdEntered_ = false;
	finished_ = false;
}

void BossRetreatAttack::Update(float dt) {

	if (phase_ == Phase::None) {
		return;
	}

	timer_ += dt;

	switch (phase_) {
	case Phase::MoveOut:
	{
		if (timer_ < params_.shrinkTime) {
			float u = (params_.shrinkTime <= 0.0f) ? 1.0f : (timer_ / params_.shrinkTime);
			u = MyMath::Clamp01(u);

			float eXZ = MyMath::EaseInOutCubic(u);
			float eY = MyMath::EaseOutCubic(u);

			float factorXZ = MyMath::Lerp(1.0f, params_.minScaleXZ, eXZ);
			float factorY = MyMath::Lerp(1.0f, params_.minScaleY, eY);

			ApplyScaleFactor(factorXZ, factorY);
			translate_ = startPos_;
			break;
		}

		float moveT = timer_ - params_.shrinkTime;
		float u = (params_.moveTime <= 0.0f) ? 1.0f : (moveT / params_.moveTime);
		u = MyMath::Clamp01(u);

		float e = MyMath::EaseInOutCubic(u);
		ApplyScaleFactor(params_.minScaleXZ, params_.minScaleY);
		translate_ = MyMath::Lerp(startPos_, backPos_, e);

		if (u >= 1.0f) {
			phase_ = Phase::StayUnflatten;
			timer_ = 0.0f;
		}
		break;
	}

	case Phase::StayUnflatten:
	{
		float u = (params_.unflattenTime <= 0.0f) ? 1.0f : (timer_ / params_.unflattenTime);
		u = MyMath::Clamp01(u);

		float e = MyMath::EaseInOutCubic(u);
		float factorXZ = MyMath::Lerp(params_.minScaleXZ, 1.0f, e);
		float factorY = MyMath::Lerp(params_.minScaleY, 1.0f, e * 0.5f);

		ApplyScaleFactor(factorXZ, factorY);
		translate_ = backPos_;

		if (u >= 1.0f) {
			phase_ = Phase::StayHold;
			timer_ = 0.0f;
			holdEntered_ = true;
			ApplyScaleFactor(1.0f, 1.0f);
		}
		break;
	}

	case Phase::StayHold:
		translate_ = backPos_;
		ApplyScaleFactor(1.0f, 1.0f);

		if (requestReturn_) {
			phase_ = Phase::Return;
			timer_ = 0.0f;
		}
		break;

	case Phase::Return:
	{
		if (timer_ < params_.flattenTime) {
			float u = (params_.flattenTime <= 0.0f) ? 1.0f : (timer_ / params_.flattenTime);
			u = MyMath::Clamp01(u);

			float eXZ = MyMath::EaseInOutCubic(u);
			float eY = MyMath::EaseOutCubic(u);

			float factorXZ = MyMath::Lerp(1.0f, params_.minScaleXZ, eXZ);
			float factorY = MyMath::Lerp(1.0f, params_.minScaleY, eY);

			ApplyScaleFactor(factorXZ, factorY);
			translate_ = backPos_;
			break;
		}

		float moveT = timer_ - params_.flattenTime;
		if (moveT < params_.moveTime) {
			float u = (params_.moveTime <= 0.0f) ? 1.0f : (moveT / params_.moveTime);
			u = MyMath::Clamp01(u);

			float e = MyMath::EaseInOutCubic(u);
			ApplyScaleFactor(params_.minScaleXZ, params_.minScaleY);
			translate_ = MyMath::Lerp(backPos_, startPos_, e);
			break;
		}

		float growT = moveT - params_.moveTime;
		float u = (params_.growTime <= 0.0f) ? 1.0f : (growT / params_.growTime);
		u = MyMath::Clamp01(u);

		float eXZ = MyMath::EaseInOutCubic(u);
		float eY = MyMath::EaseOutCubic(u);

		float factorXZ = MyMath::Lerp(params_.minScaleXZ, 1.0f, eXZ);
		float factorY = MyMath::Lerp(params_.minScaleY, 1.0f, eY * 0.5f);

		ApplyScaleFactor(factorXZ, factorY);
		translate_ = startPos_;

		if (u >= 1.0f) {
			ApplyScaleFactor(1.0f, 1.0f);
			translate_ = startPos_;
			invulnerable_ = false;
			phase_ = Phase::None;
			timer_ = 0.0f;
			finished_ = true;
		}
		break;
	}

	default:
		break;
	}
}

void BossRetreatAttack::Start(const Vector3& startPos, const Vector3& baseBodyScale, const Vector3& baseArmScale) {

	startPos_ = startPos;
	baseBodyScale_ = baseBodyScale;
	baseArmScale_ = baseArmScale;

	backPos_ = startPos_;
	backPos_.z += params_.backZOffset;
	backPos_.y += params_.upOffset;

	translate_ = startPos_;
	bodyScale_ = baseBodyScale_;
	armScale_ = baseArmScale_;

	phase_ = Phase::MoveOut;
	timer_ = 0.0f;
	invulnerable_ = true;
	requestReturn_ = false;
	holdEntered_ = false;
	finished_ = false;
}

void BossRetreatAttack::ForceEnd() {

	translate_ = startPos_;
	ApplyScaleFactor(1.0f, 1.0f);

	phase_ = Phase::None;
	timer_ = 0.0f;
	invulnerable_ = false;
	requestReturn_ = false;
	holdEntered_ = false;
	finished_ = true;
}

void BossRetreatAttack::RequestReturn() {
	requestReturn_ = true;
}

bool BossRetreatAttack::ConsumeHoldEntered() {

	if (!holdEntered_) {
		return false;
	}
	holdEntered_ = false;
	return true;
}

bool BossRetreatAttack::ConsumeFinished() {

	if (!finished_) {
		return false;
	}
	finished_ = false;
	return true;
}

void BossRetreatAttack::ApplyScaleFactor(float factorXZ, float factorY) {

	const float safeXZ = std::max(0.001f, factorXZ);

	bodyScale_ = {
		baseBodyScale_.x * safeXZ,
		baseBodyScale_.y * factorY,
		baseBodyScale_.z * safeXZ
	};

	armScale_ = {
		baseArmScale_.x * safeXZ,
		baseArmScale_.y * factorY,
		baseArmScale_.z * safeXZ
	};
}