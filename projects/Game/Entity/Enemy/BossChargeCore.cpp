#include "BossChargeCore.h"
#include "Game/Entity/Player/PlayerBullet.h"
#include "Engine/Base/System/System.h"

void BossChargeCore::Init(Camera* camera) {

	camera_ = camera;

	// 芯
	coreObj_ = std::make_unique<Object3d>();
	coreObj_->Init("object3d_chargeCore", BlendType::BLEND_NONE);
	coreObj_->SetModel("EnemyChargeCore.obj");
	coreObj_->SetDefaultCamera(camera_);
	coreObj_->SetScale(coreScale_);
	coreObj_->SetColor({ 0.82f, 0.93f, 1.0f, 1.0f });
}

void BossChargeCore::Update(float dt) {

	if (!active_) {
		return;
	}

	pulseTime_ += dt;

	// 回転はかなり弱くする
	rotY_ += dt * 0.04f;

	float hpRate = (maxHp_ > 0) ? static_cast<float>(hp_) / static_cast<float>(maxHp_) : 0.0f;
	hpRate = std::clamp(hpRate, 0.0f, 1.0f);

	// ----------------------------- //
	// チャージ経過でどんどん大きくする
	const float growDuration = 3.5f;
	float growT = std::clamp(pulseTime_ / growDuration, 0.0f, 1.0f);

	// 後半ほど迫ってくる感じを出す
	float growEase = growT * growT * (3.0f - 2.0f * growT);

	// 最小倍率 -> 最大倍率
	float growScale = 0.72f + (1.65f - 0.72f) * growEase;

	// HPが減っている時は少し不安定さを足す
	float unstable = (1.0f - hpRate) * 0.04f * std::sin(pulseTime_ * 14.0f);

	float finalScale = growScale + unstable;

	Vector4 coreColor{
		0.86f + 0.10f * growT,                 // 時間経過で少し白く
		0.94f + 0.03f * growT,
		1.00f,
		1.0f
	};

	if (coreObj_) {
		coreObj_->SetTranslate(worldPos_);
		coreObj_->SetRotate({ 0.0f, rotY_, 0.0f });
		coreObj_->SetScale({
			coreScale_.x * finalScale,
			coreScale_.y * finalScale,
			coreScale_.z * finalScale
			});
		coreObj_->SetColor(coreColor);
		coreObj_->Update();
	}
}

void BossChargeCore::Draw() {

	if (!active_) {
		return;
	}

	if (coreObj_) {
		coreObj_->Draw();
	}
}

void BossChargeCore::Activate(const Vector3& worldPos) {

	active_ = true;
	hp_ = maxHp_;
	worldPos_ = worldPos;
	pulseTime_ = 0.0f;
	rotY_ = 0.0f;

	if (coreObj_) {
		coreObj_->SetTranslate(worldPos_);
		coreObj_->SetScale(coreScale_);
	}
}

void BossChargeCore::Deactivate() {
	active_ = false;
}

void BossChargeCore::SetWorldPos(const Vector3& worldPos) {

	worldPos_ = worldPos;

	if (coreObj_) {
		coreObj_->SetTranslate(worldPos_);
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