#include "BossChargeBeam.h"
#include "Game/Entity/Player/Player.h"

void BossChargeBeam::Init(Camera* camera) {

	camera_ = camera;

	obj_ = std::make_unique<Object3d>();
	obj_->Init("object3d_chargeCore", BlendType::BLEND_NONE);
	obj_->SetModel("EnemyChargeCore.obj");
	obj_->SetDefaultCamera(camera_);
	obj_->SetColor({ 1.0f, 0.4f, 0.2f, 1.0f });
	obj_->SetScale(startScale_);
}

void BossChargeBeam::Update(float dt) {

	if (!active_ || !obj_) {
		return;
	}

	life_ += dt;
	position_ += direction_ * speed_;

	float t = std::clamp(life_ / maxLife_, 0.0f, 1.0f);
	float k = 1.0f - t;
	float ease = k * k;

	Vector3 s;
	s.x = endScale_.x + (startScale_.x - endScale_.x) * ease;
	s.y = endScale_.y + (startScale_.y - endScale_.y) * ease;
	s.z = endScale_.z + (startScale_.z - endScale_.z) * ease;

	obj_->SetTranslate(position_);
	obj_->SetScale(s);
	obj_->Update();

	if (life_ >= maxLife_) {
		Destroy();
	}
}

void BossChargeBeam::Draw() {

	if (active_ && obj_) {
		obj_->Draw();
	}
}

void BossChargeBeam::Fire(const Vector3& startPos, const Vector3& dir) {

	active_ = true;
	hitPlayer_ = false;
	life_ = 0.0f;
	position_ = startPos;
	direction_ = MyMath::Normalize(dir);

	if (obj_) {
		obj_->SetTranslate(position_);
		obj_->SetScale(startScale_);
	}
}

void BossChargeBeam::Destroy() {
	active_ = false;
}

void BossChargeBeam::OnCollision(ICollisionObject* other) {

	if (!active_) {
		return;
	}

	if (other->GetCollisionLayer() == CollisionLayer::Player) {
		hitPlayer_ = true;
		if (life_ >= minLife_) {
			Destroy();
		}
	}
}