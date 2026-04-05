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

	useCurve_ = false;
	curveT_ = 0.0f;
	lastCurveT_ = 0.0f;

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

void BossMissile::SpawnCurve(
	const Vector3& startPos,
	const Vector3& controlPos,
	const Vector3& endPos,
	float duration) {

	transform_.translate = startPos;
	transform_.rotate = { 0.0f, 0.0f, 0.0f };

	curveStart_ = startPos;
	curveControl_ = controlPos;
	curveEnd_ = endPos;

	useCurve_ = true;
	curveT_ = 0.0f;
	curveDuration_ = std::max(0.01f, duration);

	lastCurveT_ = 0.0f;
	lastCurvePos_ = startPos;

	direction_ = { 0.0f, 0.0f, 1.0f };
	speed_ = 0.0f;

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

Vector3 BossMissile::EvalQuadraticBezier(float t) const {

	float u = 1.0f - t;

	return
		curveStart_ * (u * u) +
		curveControl_ * (2.0f * u * t) +
		curveEnd_ * (t * t);
}

void BossMissile::Kill() {

	isActive_ = false;
	isAlive_ = false;
	hitPlayer_ = false;
	lifeTimer_ = 0.0f;
	useCurve_ = false;
	curveT_ = 0.0f;

	if (collisionManager_ && collisionRegistered_) {
		collisionManager_->Unregister(this);
		collisionRegistered_ = false;
	}
}

void BossMissile::Update() {

	const float dt = KomEngine::System::GetDeltaTime();

	if (!isAlive_ || !object3d_) {
		return;
	}

	if (useCurve_) {

		lastCurveT_ = curveT_;
		lastCurvePos_ = transform_.translate;

		curveT_ += dt / curveDuration_;
		float t = std::clamp(curveT_, 0.0f, 1.0f);

		Vector3 newPos = EvalQuadraticBezier(t);
		Vector3 moveDir = MyMath::Normalize(newPos - lastCurvePos_);

		transform_.translate = newPos;

		if (MyMath::Length(newPos - lastCurvePos_) > 0.0001f) {
			const float yaw = std::atan2(moveDir.x, moveDir.z);
			const float pitch = -std::asin(moveDir.y);
			transform_.rotate = { pitch, yaw, 0.0f };
		}

		if (t >= 1.0f) {
			Kill();
			return;
		}
	}
	else {

		transform_.translate += direction_ * speed_;

		const float yaw = std::atan2(direction_.x, direction_.z);
		const float pitch = -std::asin(direction_.y);
		transform_.rotate = { pitch, yaw, 0.0f };
	}

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
	}
	else if (other->GetCollisionLayer() == CollisionLayer::PlayerBullet) {
		Kill();
	}
}