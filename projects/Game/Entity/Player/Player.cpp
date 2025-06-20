
#include "Player.h"
#include "externals/imgui/imgui.h"

#include "Engine/Base/System/System.h"

#include <iostream>

float Player::GetRadius() const { return radius_; }

Transform Player::GetTransform() const { return transform_; }

std::vector<std::unique_ptr<PlayerBullet>>& Player::GetBullets() { return bulletObjects_; }

Player::~Player() {

	// 
	bulletObjects_.clear();
}

void Player::Init(Camera* camera) {

	camera_ = camera;

	// 自機、弾の生成
	object3d_ = std::make_unique<Object3d>();
	object3d_->Init(BlendType::BLEND_NONE);

	object3d_->SetModel("sphere.obj");
	object3d_->SetDefaultCamera(camera_);

	object3d_->SetScale({0.5f, 0.5f, 0.5f});

	// レティクルのスプライトを生成
	reticleSprite_ = std::make_unique<Sprite>();
	reticleSprite_->Init("./Resources/images/uvChecker.png", BlendType::BLEND_NONE);
	reticleSprite_->SetSize({50.0f, 50.0f});
}

void Player::Update() {

	Attack();

	// 弾の更新と削除
	for (auto it = bulletObjects_.begin(); it != bulletObjects_.end();) {
		(*it)->Update();
		(*it)->ImGuiDebug();

		++it;
	}

	object3d_->Update();

	// object3d_->SetTransform(transform_);
	object3d_->SetTranslate(transform_.translate);
	object3d_->SetRotate(transform_.rotate);

	// Move();
	// RailMove();
	UpdateReticleSprite();
}

void Player::Draw() {

	//
	// object3d_->Draw();

	for (auto& bullet : bulletObjects_) {
		bullet->Draw();
	}

	reticleSprite_->Draw();

	//// ロックオンスプライトの描画
	// if (lockedTarget_ && lockedTarget_->GetIsAlive()) {
	//	lockOnSprite_->Draw();
	// }
}

void Player::ImGuiDebug() {

	ImGui::Begin("Player");

	ImGui::SliderAngle("rotateX", &transform_.rotate.x, 0.1f);
	ImGui::SliderAngle("rotateY", &transform_.rotate.y, 0.1f);
	ImGui::SliderAngle("rotateZ", &transform_.rotate.z, 0.1f);
	ImGui::DragFloat3("translate", &transform_.translate.x, 0.1f);

	ImGui::End();
}

void Player::Attack() {

	if (System::TriggerKey(DIK_SPACE)) {

		// 弾の見た目（Object3d）を新規作成
		Object3d* bulletObject = new Object3d();
		bulletObject->Init(BlendType::BLEND_NONE);
		bulletObject->SetModel("sphere.obj");
		bulletObject->SetDefaultCamera(camera_);

		auto newBullet = std::make_unique<PlayerBullet>();
		newBullet->Init(camera_, bulletObject);
		newBullet->SetTranlate(transform_.translate);

		Vector3 direction;

		if (reticleSprite_) {
			Matrix4x4 viewMatrix = camera_->GetViewMatrix();
			Matrix4x4 projMatrix = camera_->GetProjectionMatrix();
			Matrix4x4 vpMatrix = MyMath::Multiply(viewMatrix, projMatrix);
			Matrix4x4 invVPMatrix = MyMath::Inverse4x4(vpMatrix);

			Matrix4x4 viewportMatrix = MyMath::MakeViewportMatrix(0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f);
			Matrix4x4 invViewportMatrix = MyMath::Inverse4x4(viewportMatrix);

			Vector2 spritePos = reticleSprite_->GetCenterPosition();
			Vector3 screenNear = {spritePos.x, spritePos.y, 0.0f};
			Vector3 screenFar = {spritePos.x, spritePos.y, 1.0f};

			Vector3 ndcNear = MyMath::Transform(screenNear, invViewportMatrix);
			Vector3 ndcFar = MyMath::Transform(screenFar, invViewportMatrix);

			Vector3 worldNear = MyMath::Transform(ndcNear, invVPMatrix);
			Vector3 worldFar = MyMath::Transform(ndcFar, invVPMatrix);

			direction = worldFar - worldNear;
		} else {
			direction = {0.0f, 0.0f, 1.0f};
		}

		MyMath::Normalize(direction);
		newBullet->SetDirection(direction);
		bulletObjects_.emplace_back(std::move(newBullet));
	}
}

void Player::Move() {

	if (System::PushKey(DIK_A)) {
		transform_.translate.x -= velocity_;
	}

	if (System::PushKey(DIK_D)) {
		transform_.translate.x += velocity_;
	}

	if (System::PushKey(DIK_W)) {
		transform_.translate.y += velocity_;
	}

	if (System::PushKey(DIK_S)) {
		transform_.translate.y -= velocity_;
	}
}

void Player::RailMove() { transform_.translate.z += velocity_; }

void Player::UpdateReticleSprite() {

	// レティクル操作（十字キー）
	Vector2 reticlePos = reticleSprite_->GetPosition();
	float moveSpeed = 10.0f;

	if (System::PushKey(DIK_LEFT)) {
		reticlePos.x -= moveSpeed;
	}
	if (System::PushKey(DIK_RIGHT)) {
		reticlePos.x += moveSpeed;
	}
	if (System::PushKey(DIK_UP)) {
		reticlePos.y -= moveSpeed;
	}
	if (System::PushKey(DIK_DOWN)) {
		reticlePos.y += moveSpeed;
	}

	// 画面外に出ないよう制限（1280x720前提）
	reticlePos.x = std::clamp(reticlePos.x, 0.0f, 1280.0f);
	reticlePos.y = std::clamp(reticlePos.y, 0.0f, 720.0f);
	reticleSprite_->SetPosition(reticlePos);

	reticleSprite_->Update();
}