#pragma once

#include "Engine/Base/3d/Object3d/Object3d.h"

class BossDeathController {

public:

	enum class Phase {
		None,
		PreFall,        // ビリビリして溜める
		FinalExplosion, // 大爆発
		Falling,        // 落下
		Landed
	};

public:

	void Start(const Transform& currentTransform);

	void Update(float dt, Transform& transform);

	bool IsStarted() const { return started_; }
	bool HasLanded() const { return hasLanded_; }

	Phase GetPhase() const { return phase_; }

	// DamageShake側で使う撃破中シェイク量
	Vector3 CalcShakeOffset() const;

	// カメラシェイク要求
	bool ConsumeFinalExplosionShakeRequest();
	bool ConsumeLandingShakeRequest();

private:

	void EmitDeathElectricParticles(const Vector3& bossPos);
	void TriggerFinalExplosion(const Vector3& bossPos);

private:

	// フェーズ
	Phase phase_ = Phase::None;

	bool started_ = false;
	bool hasLanded_ = false;
	bool finalExplosionDone_ = false;

	float deathEffectTimer_ = 0.0f;
	float deathEffectDuration_ = 1.0f;

	float finalExplosionTimer_ = 0.0f;
	float finalExplosionDuration_ = 0.20f;

	float deathSparkTimer_ = 0.0f;
	float deathSparkInterval_ = 0.06f;

	float fallVelY_ = 0.0f;
	float gravityY_ = -0.006f;
	float groundY_ = -5.0f;

	float fallRotateStart_ = 0.0f;
	float fallRotateEnd_ = -1.2f;

	float fallShakeTime_ = 0.0f;
	float fallShakeAmplitude_ = 0.25f;

	bool requestFinalExplosionShake_ = false;
	bool requestLandingShake_ = false;
};