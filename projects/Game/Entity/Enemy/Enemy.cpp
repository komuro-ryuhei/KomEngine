#include "Enemy.h"
#include <chrono>
#include <random>

#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif // _DEBUG

float Enemy::GetRadius() const { return radius_; }
bool Enemy::GetIsAlive() const { return isAlive_; }
Vector3 Enemy::GetTranslate() { return transform_.translate; }
void Enemy::SetTranslate(Vector3 translate) { transform_.translate = translate; }

void Enemy::Init(Camera* camera, Object3d* object3d) {

	camera_ = camera;
	object3d_ = object3d;

	object3d_->Init();

	transform_.translate = {0.0f, 0.0f, 10.0f};
	object3d_->SetTranslate(transform_.translate);

	// 初期移動方向をランダムに設定
	SetRandomDirection();
	moveTimer_ = 0;
}

void Enemy::Update() {

	// 点滅処理
	if (isBlinking_) {
		blinkCounter_++;
		if (blinkCounter_ >= blinkDuration_) {
			isBlinking_ = false;
			blinkCounter_ = 0;
		}
	}

	if (transform_.translate.z >= 10.0f) {
		transform_.translate.z -= velocity_;
	}

	// 移動
	Move();

	object3d_->SetTranslate(transform_.translate);
	object3d_->Update();
}

void Enemy::Draw() {

	if (isBlinking_ && (blinkCounter_ / 5) % 2 == 0) {
		return;
	}

	object3d_->Draw();
}

void Enemy::ImGuiDebug() {

#ifdef _DEBUG

	ImGui::Begin("Enemy");
	ImGui::DragFloat3("translate", &transform_.translate.x, 0.01f);
	ImGui::End();

#endif // _DEBUG
}

void Enemy::OnHit() {

	isBlinking_ = true;
	blinkCounter_ = 0;
	isAlive_ = false;
}

void Enemy::Move() {

	// 一定時間（今は5秒）経過したら新しい方向を決定
	if (moveTimer_ >= moveDuration_) {
		SetRandomDirection();
		moveTimer_ = 0;
	}

	// 移動処理
	transform_.translate += moveDirection_ * velocity_;

	// 範囲外チェック
	CheckBounds();

	moveTimer_++;
}

void Enemy::SetRandomDirection() {

	// 乱数生成
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

	// ランダムな方向を決定
	moveDirection_ = {dist(gen), dist(gen), dist(gen)};

	// 正規化
	float length = std::sqrt(moveDirection_.x * moveDirection_.x + moveDirection_.y * moveDirection_.y + moveDirection_.z * moveDirection_.z);

	if (length > 0.0f) {
		moveDirection_.x /= length;
		moveDirection_.y /= length;
		moveDirection_.z /= length;
	}
}

void Enemy::CheckBounds() {

	// 範囲外に出たら方向を変える
	if (transform_.translate.x < minX || transform_.translate.x > maxX || transform_.translate.y < minY || transform_.translate.y > maxY || transform_.translate.z < minZ ||
	    transform_.translate.z > maxZ) {

		// 範囲の中心方向に向かうように補正
		Vector3 center = {(minX + maxX) / 2.0f, (minY + maxY) / 2.0f, (minZ + maxZ) / 2.0f};
		moveDirection_ = center - transform_.translate;

		// 正規化
		float length = std::sqrt(moveDirection_.x * moveDirection_.x + moveDirection_.y * moveDirection_.y + moveDirection_.z * moveDirection_.z);
		if (length > 0.0f) {
			moveDirection_.x /= length;
			moveDirection_.y /= length;
			moveDirection_.z /= length;
		}
		// 方向変更時にタイマーリセット
		moveTimer_ = 0;
	}
}