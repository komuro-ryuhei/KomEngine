#include "ClearSequenceController.h"

#include "Game/Entity/Enemy/BossEnemy.h"
#include "Game/UI/ResultImage.h"

#include "Engine/Base/System/System.h"
#include "Engine/Base/Particle/ParticleManager.h"

#include "Engine/Base/Camera/Camera.h"

void ClearSequenceController::Start(Camera* camera) {

	started_ = true;
	resultStarted_ = false;
	resultStartedRequest_ = false;

	sequenceTimer_ = 0.0f;

	explosionStep_ = 0;
	explosionTimer_ = 0.0f;

	if (camera) {
		camera->StartShake(CameraShakeType::Large);
	}
}

void ClearSequenceController::Update(
	float dt,
	BossEnemy* boss,
	Camera* camera,
	ResultImage* result
) {

	if (!started_) {
		return;
	}

	sequenceTimer_ += dt;
	explosionTimer_ += dt;

	// 連鎖爆発を3段階で出す
	if (explosionStep_ < 3 && explosionTimer_ >= explosionInterval_) {
		explosionTimer_ = 0.0f;

		TriggerExplosionStep(
			explosionStep_,
			boss,
			camera
		);

		++explosionStep_;
	}

	// 少し待ってからリザルト表示
	if (!resultStarted_ && sequenceTimer_ >= resultStartTime_) {

		resultStarted_ = true;
		resultStartedRequest_ = true;

		if (result) {
			result->StartSlideIn();
		}
	}
}

bool ClearSequenceController::ConsumeResultStartedRequest() {

	if (!resultStartedRequest_) {
		return false;
	}

	resultStartedRequest_ = false;
	return true;
}

void ClearSequenceController::TriggerExplosionStep(
	int step,
	BossEnemy* boss,
	Camera* camera
) {

	if (!boss) {
		return;
	}

	auto* pm = KomEngine::System::GetParticleManager();
	if (!pm) {
		return;
	}

	Vector3 center = boss->GetTranslate();
	Vector3 left = boss->GetLeftHandWorldPos();
	Vector3 right = boss->GetRightHandWorldPos();

	switch (step) {

	case 0:
		if (pm->Exists("explosion")) {
			pm->Emit("explosion", center, 55);
		}
		if (pm->Exists("hit")) {
			pm->Emit("hit", center, 24);
		}
		if (pm->Exists("ring")) {
			pm->Emit("ring", center, 1);
		}
		if (pm->Exists("dust")) {
			pm->Emit("dust", center, 16);
		}
		break;

	case 1:
		if (pm->Exists("explosion")) {
			pm->Emit("explosion", left, 24);
			pm->Emit("explosion", right, 24);
		}
		if (pm->Exists("hit")) {
			pm->Emit("hit", left, 12);
			pm->Emit("hit", right, 12);
		}
		break;

	case 2:
		if (pm->Exists("explosion")) {
			pm->Emit("explosion", center, 70);
		}
		if (pm->Exists("hit")) {
			pm->Emit("hit", center, 30);
		}
		if (pm->Exists("dust")) {
			pm->Emit("dust", center, 26);
		}
		if (pm->Exists("cylinder")) {
			pm->Emit("cylinder", center, 3);
		}

		if (camera) {
			camera->StartShake(CameraShakeType::Large);
		}
		break;

	default:
		break;
	}
}