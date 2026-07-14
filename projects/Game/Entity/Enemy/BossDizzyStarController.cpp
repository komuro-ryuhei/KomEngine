#include "BossDizzyStarController.h"

#include <cmath>

namespace {
	const char* kDizzyStarModel = "star.obj";
	constexpr float kTwoPi = 6.2831853f;
}

void BossDizzyStarController::Init(Camera* camera) {

	for (size_t i = 0; i < stars_.size(); ++i) {

		auto& star = stars_[i];

		star.obj = std::make_unique<Object3d>();
		star.obj->Init(BlendType::BLEND_NONE);
		star.obj->SetModel(kDizzyStarModel);
		star.obj->SetDefaultCamera(camera);
		star.obj->SetScale({ starScale_, starScale_, starScale_ });

		const float count = static_cast<float>(stars_.size());
		star.angle = (static_cast<float>(i) / count) * kTwoPi;
		star.phaseOffset = star.angle;

		// 初期状態は非表示
		star.obj->SetScale({ 0.0f, 0.0f, 0.0f });
		star.obj->Update();
	}
}

void BossDizzyStarController::SetActive(bool active) {

	if (active_ == active) {
		return;
	}

	active_ = active;

	if (active_) {
		timer_ = 0.0f;
	}
	else {
		for (auto& star : stars_) {
			if (star.obj) {
				star.obj->SetScale({ 0.0f, 0.0f, 0.0f });
				star.obj->Update();
			}
		}
	}
}

void BossDizzyStarController::Update(float dt, const Vector3& bossPos) {

	if (!active_) {
		return;
	}

	timer_ += dt;

	const float count = static_cast<float>(stars_.size());

	for (size_t i = 0; i < stars_.size(); ++i) {

		auto& star = stars_[i];

		if (!star.obj) {
			continue;
		}

		const float baseAngle = (static_cast<float>(i) / count) * kTwoPi;
		const float angle = baseAngle + timer_ * orbitSpeed_;

		Vector3 pos = bossPos;
		pos.x += std::cos(angle) * orbitRadiusX_;
		pos.z += std::sin(angle) * orbitRadiusZ_;
		pos.y += height_;

		pos.y += std::sin(timer_ * floatSpeed_ + star.phaseOffset) * floatAmp_;

		star.obj->SetTranslate(pos);

		star.obj->SetRotate({
			0.0f,
			timer_ * 2.5f + star.phaseOffset,
			timer_ * 4.0f + star.phaseOffset
			});

		const float depthScale = 1.0f + std::sin(angle) * 0.18f;
		const float scale = starScale_ * depthScale;

		star.obj->SetScale({ scale, scale, scale });
		star.obj->Update();
	}
}

void BossDizzyStarController::Draw() {

	if (!active_) {
		return;
	}

	for (auto& star : stars_) {
		if (star.obj) {
			star.obj->Draw();
		}
	}
}