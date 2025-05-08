
#include "Player.h"
#include "externals/imgui/imgui.h"

#include "Engine/Base/System/System.h"

#include <iostream>

float Player::GetRadius() const { return radius_; }
std::vector<std::unique_ptr<PlayerBullet>>& Player::GetBullets() { return bullets_; }

Player::~Player() {

	for (auto bulletObject : bulletObjects_) {
		delete bulletObject;
	}
	bulletObjects_.clear();
}

void Player::Init(Camera* camera, Sprite* sprite) {

	camera_ = camera;
	lockOnSprite_ = sprite;

	// 自機、弾の生成
	object3d_ = std::make_unique<Object3d>();
	object3d_->Init();

	object3d_->SetModel("sphere.obj");
	object3d_->SetDefaultCamera(camera_);

	object3d_->SetScale({0.5f, 0.5f, 0.5f});

	// 弾オブジェクトの生成
	for (int i = 0; i < 10; ++i) {
		auto bulletObject = new Object3d();
		bulletObject->Init();
		bulletObject->SetModel("sphere.obj");
		bulletObject->SetDefaultCamera(camera_);
		bulletObjects_.push_back(bulletObject);
	}
}

void Player::Update() {

	// if (lockedTarget_ && lockedTarget_->GetIsAlive()) {
	//	// 必要な行列を取得
	//	Matrix4x4 viewMatrix = camera_->GetViewMatrix();
	//	Matrix4x4 projMatrix = camera_->GetProjectionMatrix();
	//	Matrix4x4 viewportMatrix = MyMath::MakeViewportMatrix(0.0f, 0.0f, 1280, 720, 0.0f, 1.0f);

	//	// ロックオン対象の座標をスクリーン座標に変換
	//	Vector3 targetPosition = lockedTarget_->GetTranslate();
	//	Vector2 screenPosition = MyMath::WorldToScreen(targetPosition, viewMatrix, projMatrix, viewportMatrix);

	//	// スプライトの位置を更新
	//	lockOnSprite_->SetPosition(screenPosition);
	//}

	 Attack();

	// 弾の更新と削除
	for (auto it = bullets_.begin(); it != bullets_.end();) {
		(*it)->Update();
		(*it)->ImGuiDebug();

		++it;
	}

	object3d_->Update();

	// object3d_->SetTransform(transform_);
	object3d_->SetTranslate(transform_.translate);
	object3d_->SetRotate(transform_.rotate);

	Move();
}

void Player::Draw() {

	//
	object3d_->Draw();

	for (auto& bullet : bullets_) {
		bullet->Draw();
	}

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

	if (System::TriggerKey(DIK_SPACE) && !bulletObjects_.empty()) {
		Object3d* bulletObject = bulletObjects_.back();
		bulletObjects_.pop_back();

		auto newBullet = std::make_unique<PlayerBullet>();
		newBullet->Init(camera_, bulletObject);
		newBullet->SetTranlate(transform_.translate);

		// ロックオン対象が有効か確認
		 if (lockedTarget_ && lockedTarget_->GetIsAlive()) {
			// ロックオン対象に向けて弾を発射
			Vector3 direction = lockedTarget_->GetTranslate() - transform_.translate;
			MyMath::Normalize(direction);
			newBullet->SetDirection(direction);
		 } else {
			// ロックオン対象が無効ならデフォルトの前方向に発射
			lockedTarget_ = nullptr;
			Vector3 straightDirection = {0.0f, 0.0f, 1.0f};
			MyMath::Normalize(straightDirection);
			newBullet->SetDirection(straightDirection);
		 }

		Vector3 straightDirection = {0.0f, 0.0f, 1.0f};
		MyMath::Normalize(straightDirection);
		newBullet->SetDirection(straightDirection);

		bullets_.emplace_back(std::move(newBullet));
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

 void Player::LockOnTarget(std::vector<std::unique_ptr<Enemy>>& enemies) {

	float closestDistance = (std::numeric_limits<float>::max)();
	Enemy* closestEnemy = nullptr;

	for (auto& enemy : enemies) {
		if (!enemy->GetIsAlive())
			continue;

		float distance = MyMath::CalculateDistance(transform_.translate, enemy->GetTranslate());
		if (distance < closestDistance) {
			closestDistance = distance;
			closestEnemy = enemy.get();
		}
	}
	// 最も近い敵をロックオン
	lockedTarget_ = closestEnemy;
 }