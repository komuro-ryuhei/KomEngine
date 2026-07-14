#pragma once

#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Camera/Camera.h"
#include "struct.h"

class BossEnrageTransitionController {

public:

	void Start(float duration, const Vector3& startPos);

	void Update(
		float dt,
		Transform& transform,
		Object3d* body,
		Object3d* leftArm,
		Object3d* rightArm,
		Camera* camera,
		bool isEnraged
	);

	bool IsActive() const { return active_; }

	bool ConsumeFinished();

private:

	enum class Phase {
		None,
		Knockback,
		Wait,
		Recover,
	};

private:

	void SetColor(
		Object3d* body,
		Object3d* leftArm,
		Object3d* rightArm,
		const Vector4& color
	);

	void EmitRecoverShockwave(const Vector3& pos, Camera* camera);

private:

	Phase phase_ = Phase::None;

	bool active_ = false;
	bool finishedRequest_ = false;
	bool shockwaveEmitted_ = false;

	float timer_ = 0.0f;
	float duration_ = 0.0f;

	float knockbackDuration_ = 0.25f;
	float waitDuration_ = 1.50f;
	float recoverDuration_ = 0.35f;

	Vector3 startPos_{};
	Vector3 knockbackPos_{};

	float knockbackDistance_ = 3.5f;
	float knockbackLift_ = 1.0f;

	float shakeAmplitude_ = 0.12f;
	float shakeFrequency_ = 40.0f;

	float flashSpeed_ = 10.0f;
};