#include "BossChargeCore.h"
#include "Game/Entity/Player/PlayerBullet.h"
#include "Engine/Base/System/System.h"

void BossChargeCore::Init(Camera* camera) {

	camera_ = camera;

	obj_ = std::make_unique<Object3d>();
	obj_->Init(BlendType::BLEND_ADD);
	obj_->SetModel("EnemyChargeCore.obj");
	obj_->SetDefaultCamera(camera_);
	obj_->SetScale(scale_);

	// 青白い中心光
	obj_->SetColor({ 0.88f, 0.96f, 1.0f, 1.0f });
}

void BossChargeCore::Update(float dt) {

	if (!active_ || !obj_) {
		return;
	}

	pulseTime_ += dt;

	// 回転はかなり弱める
	rotY_ += dt * 0.6f;

	obj_->SetTranslate(worldPos_);
	obj_->SetRotate({ 0.0f, rotY_, 0.0f });

	float hpRate = (maxHp_ > 0) ? static_cast<float>(hp_) / static_cast<float>(maxHp_) : 0.0f;
	hpRate = std::clamp(hpRate, 0.0f, 1.0f);

	// HPが減るほど少し不安定になる
	float pulse =
		1.0f +
		std::sin(pulseTime_ * 8.0f) * 0.08f +
		(1.0f - hpRate) * 0.10f;

	float s = pulse;
	obj_->SetScale({
		scale_.x * s,
		scale_.y * s,
		scale_.z * s
		});

	// 青寄り
	Vector4 coreColor{
		0.90f + (1.00f - 0.90f) * (1.0f - hpRate),
		0.96f,
		1.00f,
		1.0f
	};
	obj_->SetColor(coreColor);

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