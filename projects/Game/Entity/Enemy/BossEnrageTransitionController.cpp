#include "BossEnrageTransitionController.h"

#include "Engine/Base/System/System.h"
#include "Engine/Base/Particle/ParticleManager.h"
#include "Engine/lib/Math/MyMath.h"

#include <algorithm>
#include <cmath>

void BossEnrageTransitionController::Start(float duration, const Vector3& startPos) {

	active_ = true;
	finishedRequest_ = false;
	shockwaveEmitted_ = false;

	phase_ = Phase::Knockback;
	timer_ = 0.0f;
	duration_ = duration;

	startPos_ = startPos;

	knockbackDuration_ = std::min(0.25f, duration * 0.18f);
	recoverDuration_ = std::min(0.35f, duration * 0.18f);
	waitDuration_ = std::max(0.0f, duration - knockbackDuration_ - recoverDuration_);

	knockbackPos_ = startPos_;
	knockbackPos_.z += knockbackDistance_;
	knockbackPos_.y += knockbackLift_;
}

void BossEnrageTransitionController::Update(
	float dt,
	Transform& transform,
	Object3d* body,
	Object3d* leftArm,
	Object3d* rightArm,
	Camera* camera,
	bool isEnraged
) {

	if (!active_) {
		return;
	}

	timer_ += dt;

	const Vector4 baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	const Vector4 redColor = { 1.0f, 0.25f, 0.25f, 1.0f };

	switch (phase_) {

	case Phase::Knockback:
	{
		float t = (knockbackDuration_ > 0.0f)
			? timer_ / knockbackDuration_
			: 1.0f;

		t = std::clamp(t, 0.0f, 1.0f);

		// 勢いよく後ろへ飛ぶ
		float ease = 1.0f - (1.0f - t) * (1.0f - t);

		transform.translate = MyMath::Lerp(startPos_, knockbackPos_, ease);

		SetColor(body, leftArm, rightArm, baseColor);

		if (t >= 1.0f) {
			phase_ = Phase::Wait;
			timer_ = 0.0f;
		}
		break;
	}

	case Phase::Wait:
	{
		Vector3 pos = knockbackPos_;

		// 小刻みに震える
		float sx = std::sin(timer_ * shakeFrequency_) * shakeAmplitude_;
		float sz = std::cos(timer_ * (shakeFrequency_ * 1.27f)) * shakeAmplitude_;

		pos.x += sx;
		pos.z += sz;

		transform.translate = pos;

		// 赤点滅
		float flash = (std::sin(timer_ * flashSpeed_) + 1.0f) * 0.5f;

		Vector4 color{
			baseColor.x + (redColor.x - baseColor.x) * flash,
			baseColor.y + (redColor.y - baseColor.y) * flash,
			baseColor.z + (redColor.z - baseColor.z) * flash,
			1.0f
		};

		SetColor(body, leftArm, rightArm, color);

		if (timer_ >= waitDuration_) {
			phase_ = Phase::Recover;
			timer_ = 0.0f;

			if (!shockwaveEmitted_) {
				EmitRecoverShockwave(transform.translate, camera);
				shockwaveEmitted_ = true;
			}
		}
		break;
	}

	case Phase::Recover:
	{
		float t = (recoverDuration_ > 0.0f)
			? timer_ / recoverDuration_
			: 1.0f;

		t = std::clamp(t, 0.0f, 1.0f);

		float ease = t * t * (3.0f - 2.0f * t);

		transform.translate = MyMath::Lerp(knockbackPos_, startPos_, ease);

		SetColor(body, leftArm, rightArm, baseColor);

		if (t >= 1.0f) {
			transform.translate = startPos_;

			// 演出終了後に怒りモデルへ切り替える
			if (isEnraged && body) {
				body->SetModel("BossEnemyCore_Enrage.obj");
			}

			phase_ = Phase::None;
			active_ = false;
			timer_ = 0.0f;
			shockwaveEmitted_ = false;
			finishedRequest_ = true;
		}
		break;
	}

	default:
		phase_ = Phase::None;
		active_ = false;
		timer_ = 0.0f;
		shockwaveEmitted_ = false;
		finishedRequest_ = true;
		break;
	}
}

bool BossEnrageTransitionController::ConsumeFinished() {

	if (!finishedRequest_) {
		return false;
	}

	finishedRequest_ = false;
	return true;
}

void BossEnrageTransitionController::SetColor(
	Object3d* body,
	Object3d* leftArm,
	Object3d* rightArm,
	const Vector4& color
) {

	if (body) {
		body->SetColor(color);
	}

	if (leftArm) {
		leftArm->SetColor(color);
	}

	if (rightArm) {
		rightArm->SetColor(color);
	}
}

void BossEnrageTransitionController::EmitRecoverShockwave(const Vector3& pos, Camera* camera) {

	auto* pm = KomEngine::System::GetParticleManager();
	if (pm) {
		if (pm->Exists("ring")) {
			pm->Emit("ring", pos, 1);
		}

		if (pm->Exists("dust")) {
			pm->Emit("dust", pos, 18);
		}
	}

	if (camera) {
		camera->StartShake(CameraShakeType::Large);
	}
}