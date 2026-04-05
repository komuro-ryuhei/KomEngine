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
		(*missiles_)[i]->Update();
	}

	if (timer_ >= params_.telegraphTime) {
		phase_ = Phase::Launch;
		timer_ = 0.0f;

		const Vector3 playerPos = player_->GetTransform().translate;

		for (int i = 0; i < count; ++i) {
			Vector3 from = (*missiles_)[i]->GetTranslate();

			// プレイヤー現在位置
			Vector3 end = playerPos;

			// ミサイルごとに左右どちらへ膨らませるか決める
			float sideSign = 0.0f;
			if (count > 1) {
				float t = static_cast<float>(i) / static_cast<float>(count - 1); // 0..1
				sideSign = (t - 0.5f) * 2.0f; // -1 .. +1
			}

			// start→end 方向
			Vector3 forward = MyMath::Normalize(end - from);

			// 横方向ベクトル（XZ平面ベース）
			Vector3 side = { forward.z, 0.0f, -forward.x };
			if (MyMath::Length(side) < 0.0001f) {
				side = { 1.0f, 0.0f, 0.0f };
			}
			side = MyMath::Normalize(side);

			// 中間点を外側にずらす
			Vector3 mid = (from + end) * 0.5f;
			Vector3 control =
				mid +
				side * (params_.curveSideOffset * sideSign) +
				Vector3{ 0.0f, params_.curveUpOffset, 0.0f };

			(*missiles_)[i]->SpawnCurve(from, control, end, params_.curveDuration);
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

		m->Update();

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