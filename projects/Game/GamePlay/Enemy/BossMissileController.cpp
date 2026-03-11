#include "BossMissileController.h"

#include "Game/Entity/Enemy/BossMissile.h"
#include "Game/Entity/Enemy/BossEnemy.h"
#include "Game/Entity/Player/Player.h"
#include "Engine/lib/Math/MyMath.h"

void BossMissileController::Init() {

	phase_ = Phase::None;
	timer_ = 0.0f;
	hitPlayer_ = false;
}

void BossMissileController::Update(float dt) {

	if (phase_ == Phase::None) {
		return;
	}

	switch (phase_) {
	case Phase::Telegraph:
		UpdateTelegraph(dt);
		break;
	case Phase::Launch:
		UpdateLaunch(dt);
		break;
	default:
		break;
	}
}

void BossMissileController::Start() {

	if (!boss_ || !player_ || !missiles_) {
		return;
	}

	phase_ = Phase::Telegraph;
	timer_ = 0.0f;
	hitPlayer_ = false;

	const Vector3 bossPos = boss_->GetTranslate();

	const int count = static_cast<int>(std::min<size_t>(4, missiles_->size()));
	for (int i = 0; i < count; ++i) {
		float t = (count <= 1) ? 0.0f : static_cast<float>(i) / static_cast<float>(count - 1);
		float rad = t * MyMath::GetPI();

		Vector3 offset{};
		offset.x = std::cos(rad) * params_.radius;
		offset.y = std::sin(rad) * params_.radius + params_.height;
		offset.z = 0.0f;

		(*missiles_)[i]->Spawn(bossPos + offset, { 0.0f, 0.0f, 1.0f }, 0.0f);
	}
}

void BossMissileController::ForceEnd() {

	if (missiles_) {
		for (auto& m : *missiles_) {
			if (m) {
				m->Kill();
			}
		}
	}

	phase_ = Phase::None;
	timer_ = 0.0f;
}

void BossMissileController::UpdateTelegraph(float dt) {

	if (!boss_ || !missiles_) {
		ForceEnd();
		return;
	}

	timer_ += dt;

	const Vector3 bossPos = boss_->GetTranslate();
	const int count = static_cast<int>(std::min<size_t>(4, missiles_->size()));

	for (int i = 0; i < count; ++i) {
		float t = (count <= 1) ? 0.0f : static_cast<float>(i) / static_cast<float>(count - 1);
		float rad = t * MyMath::GetPI();

		Vector3 offset{};
		offset.x = std::cos(rad) * params_.radius;
		offset.y = std::sin(rad) * params_.radius + params_.height;
		offset.z = 0.0f;

		(*missiles_)[i]->SetTranslate(bossPos + offset);
		(*missiles_)[i]->Update(0.0f);
	}

	if (timer_ >= params_.telegraphTime) {
		phase_ = Phase::Launch;
		timer_ = 0.0f;

		const Vector3 playerPos = player_->GetTransform().translate;

		for (int i = 0; i < count; ++i) {
			Vector3 from = (*missiles_)[i]->GetTranslate();
			Vector3 dir = MyMath::Normalize(playerPos - from);
			(*missiles_)[i]->Spawn(from, dir, params_.speed);
		}
	}
}

void BossMissileController::UpdateLaunch(float dt) {

	if (!missiles_) {
		ForceEnd();
		return;
	}

	timer_ += dt;

	int aliveCount = 0;
	for (auto& m : *missiles_) {
		if (!m || !m->IsAlive()) {
			continue;
		}

		m->Update(dt);

		if (m->DidHitPlayer()) {
			hitPlayer_ = true;
		}

		if (m->IsAlive()) {
			++aliveCount;
		}
	}

	if (hitPlayer_ || aliveCount <= 0 || timer_ >= params_.launchTimeout) {
		ForceEnd();
	}
}