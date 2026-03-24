#include "BossMissile.h"

#include "Engine/Base/Camera/Camera.h"
#include "Game/Entity/Player/Player.h"
#include "Engine/lib/Math/MyMath.h"

void BossMissile::Init(Camera* camera) {

	camera_ = camera;

	object3d_ = std::make_unique<Object3d>();
	object3d_->Init(BlendType::BLEND_NONE);
	object3d_->SetModel("BossEnemyMissile.obj");
	object3d_->SetDefaultCamera(camera_);
	object3d_->SetScale({ 1.0f, 1.0f, 1.0f });
	object3d_->SetRadius(radius_ * object3d_->GetScale().x);
}

void BossMissile::Spawn(const Vector3& startPos, const Vector3& direction, float speed) {

	transform_.translate = startPos;
	transform_.rotate = { 0.0f, 0.0f, 0.0f };

	direction_ = MyMath::Normalize(direction);
	speed_ = speed;

	isAlive_ = true;
	hitPlayer_ = false;
	lifeTimer_ = 0.0f;

	if (object3d_) {
		object3d_->SetTranslate(transform_.translate);
		object3d_->SetRotate(transform_.rotate);
		object3d_->Update();
	}

	if (collisionManager_ && !collisionRegistered_) {
		collisionManager_->Register(this);
		collisionRegistered_ = true;
	}
}

void BossMissile::Kill() {

	isActive_ = false;
	isAlive_ = false;
	hitPlayer_ = false;
	lifeTimer_ = 0.0f;

	if (collisionManager_ && collisionRegistered_) {
		collisionManager_->Unregister(this);
		collisionRegistered_ = false;
	}
}

void BossMissile::Update() {

	const float dt = System::GetDeltaTime();

	if (!isAlive_ || !object3d_) {
		return;
	}

	transform_.translate += direction_ * speed_;

	// 見た目用の向き
	const float yaw = std::atan2(direction_.x, direction_.z);
	const float pitch = -std::asin(direction_.y);
	transform_.rotate = { pitch, yaw, 0.0f };

	object3d_->SetTranslate(transform_.translate);
	object3d_->SetRotate(transform_.rotate);
	object3d_->Update();

	lifeTimer_ += dt;
	if (lifeTimer_ >= maxLife_) {
		Kill();
	}
}

void BossMissile::Draw() {

	if (!isAlive_ || !object3d_) {
		return;
	}
	object3d_->Draw();
}

Vector3 BossMissile::GetCollisionPosition() const {

	if (object3d_) {
		return object3d_->GetWorldPosition();
	}
	return transform_.translate;
}

float BossMissile::GetCollisionRadius() const {

	if (!isAlive_) {
		return 0.0f;
	}
	if (object3d_) {
		return object3d_->GetRadius();
	}
	return radius_;
}

void BossMissile::OnCollision(ICollisionObject* other) {

	if (!isAlive_ || !other) {
		return;
	}

	if (other->GetCollisionLayer() == CollisionLayer::Player) {

		hitPlayer_ = true;
		Kill();
	} else if (other->GetCollisionLayer() == CollisionLayer::PlayerBullet) {
		Kill();
	}
}