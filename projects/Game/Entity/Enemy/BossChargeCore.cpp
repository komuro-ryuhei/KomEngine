#include "BossChargeCore.h"
#include "Game/Entity/Player/PlayerBullet.h"
#include "Engine/Base/System/System.h"

void BossChargeCore::Init(Camera* camera) {

	camera_ = camera;

	obj_ = std::make_unique<Object3d>();
	obj_->Init(BlendType::BLEND_ADD);
	obj_->SetModel("sphere.obj");
	obj_->SetDefaultCamera(camera_);
	obj_->SetScale(scale_);
	obj_->SetColor({ 1.0f, 0.25f, 0.25f, 1.0f });
}

void BossChargeCore::Update(float dt) {

	if (!active_ || !obj_) {
		return;
	}

	rotY_ += dt * 3.0f;
	obj_->SetTranslate(worldPos_);
	obj_->SetRotate({ 0.0f, rotY_, 0.0f });

	float hpRate = (maxHp_ > 0) ? (float)hp_ / (float)maxHp_ : 0.0f;
	hpRate = std::clamp(hpRate, 0.0f, 1.0f);

	const float s = 0.8f + (1.0f - hpRate) * 0.25f;
	obj_->SetScale({ scale_.x * s, scale_.y * s, scale_.z * s });

	obj_->Update();
}

void BossChargeCore::Draw() {

	if (active_ && obj_) {
		obj_->Draw();
	}
}

void BossChargeCore::Activate(const Vector3& worldPos) {

	active_ = true;
	hp_ = maxHp_;
	worldPos_ = worldPos;

	if (obj_) {
		obj_->SetTranslate(worldPos_);
		obj_->SetScale(scale_);
	}
}

void BossChargeCore::Deactivate() {
	active_ = false;
}

void BossChargeCore::SetWorldPos(const Vector3& worldPos) {

	worldPos_ = worldPos;
	if (obj_) {
		obj_->SetTranslate(worldPos_);
	}
}

void BossChargeCore::OnCollision(ICollisionObject* other) {

	if (!active_) {
		return;
	}

	if (other->GetCollisionLayer() != CollisionLayer::PlayerBullet) {
		return;
	}

	int dmg = 1;
	if (auto* bullet = dynamic_cast<PlayerBullet*>(other)) {
		dmg = std::max(1, bullet->GetDamage());
	}

	hp_ -= dmg;
	if (hp_ < 0) {
		hp_ = 0;
	}
}