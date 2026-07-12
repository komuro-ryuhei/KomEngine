#include "BossDeathController.h"

#include "Engine/Base/System/System.h"
#include "Engine/Base/Particle/ParticleManager.h"

#include <algorithm>
#include <cmath>

void BossDeathController::Start(const Transform& currentTransform) {

	started_ = true;
	hasLanded_ = false;

	phase_ = Phase::PreFall;

	deathEffectTimer_ = 0.0f;
	finalExplosionTimer_ = 0.0f;
	deathSparkTimer_ = 0.0f;

	finalExplosionDone_ = false;

	fallVelY_ = 0.0f;
	fallRotateStart_ = currentTransform.rotate.x;
	fallShakeTime_ = 0.0f;

	requestFinalExplosionShake_ = false;
	requestLandingShake_ = false;
}

void BossDeathController::Update(float dt, Transform& transform) {

	// -----------------------------
	// ビリビリ演出
	// -----------------------------
	if (phase_ == Phase::PreFall) {

		deathEffectTimer_ += dt;
		deathSparkTimer_ += dt;
		fallShakeTime_ += dt;

		if (deathSparkTimer_ >= deathSparkInterval_) {
			deathSparkTimer_ = 0.0f;
			EmitDeathElectricParticles(transform.translate);
		}

		// たまに軽い火花
		if (std::fmod(deathEffectTimer_, 0.22f) < dt) {
			auto* pm = KomEngine::System::GetParticleManager();
			if (pm && pm->Exists("hit")) {
				pm->Emit("hit", transform.translate, 6);
			}
		}

		if (deathEffectTimer_ >= deathEffectDuration_) {
			phase_ = Phase::FinalExplosion;
			finalExplosionTimer_ = 0.0f;
			TriggerFinalExplosion(transform.translate);
		}

		return;
	}

	// -----------------------------
	// 大爆発を少し見せる
	// -----------------------------
	if (phase_ == Phase::FinalExplosion) {

		finalExplosionTimer_ += dt;
		fallShakeTime_ += dt;

		if (finalExplosionTimer_ >= finalExplosionDuration_) {
			phase_ = Phase::Falling;
		}

		return;
	}

	// -----------------------------
	// 落下
	// -----------------------------
	if (phase_ == Phase::Falling && !hasLanded_) {

		fallShakeTime_ += dt;

		// 元の処理に合わせて、dtを掛けずに毎フレーム加算
		fallVelY_ += gravityY_;
		transform.translate.y += fallVelY_;

		float fallProgress = (transform.translate.y - groundY_) / (2.0f - groundY_);
		fallProgress = std::clamp(1.0f - fallProgress, 0.0f, 1.0f);

		float ease = fallProgress * fallProgress;
		transform.rotate.x = MyMath::Lerp(fallRotateStart_, fallRotateEnd_, ease);

		if (transform.translate.y <= groundY_) {
			transform.translate.y = groundY_;
			fallVelY_ = 0.0f;

			hasLanded_ = true;
			phase_ = Phase::Landed;

			requestLandingShake_ = true;

			auto* pm = KomEngine::System::GetParticleManager();
			if (pm && pm->Exists("dust")) {
				pm->Emit("dust", transform.translate, 120);
			}
		}
	}
}

Vector3 BossDeathController::CalcShakeOffset() const {

	if (!started_ || hasLanded_) {
		return {};
	}

	Vector3 offset{};

	if (phase_ == Phase::PreFall) {
		offset.x = std::sin(fallShakeTime_ * 85.0f) * 0.12f;
		offset.y = std::cos(fallShakeTime_ * 110.0f) * 0.06f;
		offset.z = std::cos(fallShakeTime_ * 95.0f) * 0.12f;
	}
	else if (phase_ == Phase::FinalExplosion) {
		offset.x = std::sin(fallShakeTime_ * 45.0f) * 0.25f;
		offset.z = std::cos(fallShakeTime_ * 52.0f) * 0.25f;
	}
	else {
		offset.x = std::sin(fallShakeTime_ * 40.0f) * fallShakeAmplitude_;
		offset.z = std::cos(fallShakeTime_ * 55.0f) * fallShakeAmplitude_;
	}

	return offset;
}

bool BossDeathController::ConsumeFinalExplosionShakeRequest() {

	if (!requestFinalExplosionShake_) {
		return false;
	}

	requestFinalExplosionShake_ = false;
	return true;
}

bool BossDeathController::ConsumeLandingShakeRequest() {

	if (!requestLandingShake_) {
		return false;
	}

	requestLandingShake_ = false;
	return true;
}

void BossDeathController::EmitDeathElectricParticles(const Vector3& bossPos) {

	auto* pm = KomEngine::System::GetParticleManager();
	if (!pm) {
		return;
	}

	Vector3 p{
		bossPos.x + MyMath::Rand(-2.2f, 2.2f),
		bossPos.y + MyMath::Rand(-1.2f, 2.0f),
		bossPos.z + MyMath::Rand(-2.2f, 2.2f)
	};

	if (pm->Exists("hit")) {
		pm->Emit("hit", p, 4);
	}

	if (pm->Exists("explosion") && MyMath::Rand(0.0f, 1.0f) < 0.25f) {
		pm->Emit("explosion", p, 2);
	}
}

void BossDeathController::TriggerFinalExplosion(const Vector3& bossPos) {

	if (finalExplosionDone_) {
		return;
	}

	finalExplosionDone_ = true;
	requestFinalExplosionShake_ = true;

	auto* pm = KomEngine::System::GetParticleManager();
	if (!pm) {
		return;
	}

	if (pm->Exists("explosion")) {
		pm->Emit("explosion", bossPos, 110);
	}

	if (pm->Exists("hit")) {
		pm->Emit("hit", bossPos, 55);
	}

	if (pm->Exists("cylinder")) {
		pm->Emit("cylinder", bossPos, 4);
	}

	if (pm->Exists("dust")) {
		pm->Emit("dust", bossPos, 36);
	}

	if (pm->Exists("explosion")) {
		pm->Emit("explosion", bossPos + Vector3{ 0.8f, 0.4f, 0.0f }, 26);
		pm->Emit("explosion", bossPos + Vector3{ -0.7f, 0.2f, 0.5f }, 24);
		pm->Emit("explosion", bossPos + Vector3{ 0.3f, 0.7f, -0.6f }, 20);
	}
}