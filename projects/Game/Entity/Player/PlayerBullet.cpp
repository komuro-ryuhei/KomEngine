#include "PlayerBullet.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include "Engine/Base/Particle/ParticleManager.h"

float PlayerBullet::GetRadius() const { return radius_; }

void PlayerBullet::Init(Camera* camera) {

	camera_ = camera;

	object3d_ = std::make_unique<Object3d>();
	object3d_->Init(BlendType::BLEND_NONE);
	object3d_->SetModel("PlayerBullet.obj");
	object3d_->SetDefaultCamera(camera_);
	object3d_->SetScale({ 0.1f, 0.1f, 0.1f });
	object3d_->Update();

	// トレイル用エミッター生成
	trailEmitter_ = std::make_unique<ParticleEmitter>();
	trailEmitter_->Init("trail", transform_.translate, 5);

	Deactivate();
}

void PlayerBullet::Activate(
	const Vector3& translate,
	const Vector3& direction,
	float speed,
	float radius,
	const Vector3& scale,
	int damage
) {

	isActive_ = true;
	isAlive_ = true;
	pendingKill_ = false;

	lifeTimer_ = 0.0f;

	damage_ = damage;
	speed_ = speed;
	radius_ = radius;
	direction_ = direction;

	SetScale(scale);
	SetTranlate(translate);

	if (object3d_) {
		object3d_->Update();
	}
}

void PlayerBullet::Deactivate() {

	isActive_ = false;
	isAlive_ = false;
	pendingKill_ = false;

	lifeTimer_ = 0.0f;
}

void PlayerBullet::Update() {

	if (!isAlive_) {
		return;
	}

	if (pendingKill_) {
		Deactivate();
		return;
	}

	// 弾を進める
	transform_.translate += direction_ * speed_;

	if (object3d_) {
		object3d_->SetTranslate(transform_.translate);
	}

	// トレイル
	if (trailEmitter_) {
		trailEmitter_->SetTranslate(transform_.translate);
		trailEmitter_->Update();
	}

	// 寿命処理
	lifeTimer_ += 1.0f / 60.0f;
	if (lifeTimer_ >= lifeTime_) {
		Deactivate();
		return;
	}

	if (object3d_) {
		object3d_->Update();
	}
}

void PlayerBullet::Draw() {

	if (!isAlive_) {
		return;
	}

	if (object3d_) {
		object3d_->Draw();
	}
}

void PlayerBullet::ImGuiDebug() {

#ifdef USE_IMGUI

	if (!isAlive_) {
		return;
	}

	ImGui::Begin("PlayerBullet");

	ImGui::DragFloat3("bulletTranlate", &transform_.translate.x, 0.01f);
	ImGui::DragFloat3("bulletRotate", &transform_.rotate.x, 0.01f);

	ImGui::End();

#endif
}

Vector3 PlayerBullet::GetTranslate() const { return transform_.translate; }

void PlayerBullet::SetTranlate(Vector3 translate) {

	transform_.translate = translate;

	if (object3d_) {
		object3d_->SetTranslate(translate);
	}

	if (trailEmitter_) {
		trailEmitter_->SetTranslate(translate);
	}
}

void PlayerBullet::SetDirection(const Vector3& direction) { direction_ = direction; }

void PlayerBullet::SetScale(const Vector3& s) {

	if (object3d_) {
		object3d_->SetScale(s);
	}
}

// ================= ICollisionObject の実装 ================= //

Vector3 PlayerBullet::GetCollisionPosition() const {

	return transform_.translate;
}

float PlayerBullet::GetCollisionRadius() const {

	if (!isAlive_) {
		return 0.0f;
	}

	return radius_;
}

CollisionLayer PlayerBullet::GetCollisionLayer() const {

	return CollisionLayer::PlayerBullet;
}

void PlayerBullet::OnCollision(ICollisionObject* other) {

	if (!isAlive_ || !other) {
		return;
	}

	switch (other->GetCollisionLayer()) {

	case CollisionLayer::Enemy:
		pendingKill_ = true;
		break;

	case CollisionLayer::EnemyBullet:
		pendingKill_ = true;
		break;

	case CollisionLayer::EnemyMeteor:
		pendingKill_ = true;
		break;

	case CollisionLayer::EnemyCore:
		pendingKill_ = true;
		break;

	case CollisionLayer::EnemyMissile:
		pendingKill_ = true;
		break;

	case CollisionLayer::Environment:
		pendingKill_ = true;
		break;

	default:
		break;
	}
}