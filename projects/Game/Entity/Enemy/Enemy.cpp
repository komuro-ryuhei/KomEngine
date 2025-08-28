#include "Enemy.h"
#include <chrono>
#include <random>
#include "Engine/lib/Math/MyMath.h"

#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif // _DEBUG

float Enemy::GetRadius() const { return radius_; }
bool Enemy::GetIsAlive() const { return isAlive_; }
Vector3 Enemy::GetTranslate() { return transform_.translate; }
void Enemy::SetTranslate(Vector3 translate) { transform_.translate = translate; }
std::vector<std::unique_ptr<EnemyBullet>>& Enemy::GetBullets() { return bulletObjects_; }

void Enemy::SetPlayer(Player* player) { player_ = player; }

void Enemy::SetRadius(float radius) { radius_ = radius; }

void Enemy::Init(Camera* camera, Object3d* object3d) {

	camera_ = camera;
	object3d_ = object3d;

	object3d_->Init(BlendType::BLEND_NONE);
}

void Enemy::Update() {

	Move();

	// 出現してからのタイマー進行
	if (spawnWaitTimer_ < spawnWaitDuration_) {
		spawnWaitTimer_++;
	}

	// 被弾中処理
	if (isBlinking_) {
		blinkCounter_++;
		if (blinkCounter_ >= blinkDuration_) {
			isBlinking_ = false;
			blinkCounter_ = 0;
		}
	}

	// 攻撃処理(出現待機中は攻撃しない)
	if (spawnWaitTimer_ >= spawnWaitDuration_) {
		Attack();
	}

	// 弾更新
	for (auto& bullet : bulletObjects_) {
		bullet->Update();
	}

	object3d_->SetTranslate(transform_.translate);
	object3d_->Update();
}

void Enemy::Draw() {

	// 点滅時は描画スキップ
	if (isBlinking_ && (blinkCounter_ / 5) % 2 == 0)
		return;

	object3d_->Draw();

	// 弾描画
	for (auto& bullet : bulletObjects_) {
		bullet->Draw();
	}
}

void Enemy::ImGuiDebug() {

#ifdef _DEBUG

	ImGui::Begin("Enemy");
	ImGui::DragFloat3("translate", &transform_.translate.x, 0.01f);
	ImGui::End();

	for (auto& bullet : bulletObjects_) {
		bullet->ImGuiDebug();
	}

#endif // _DEBUG
}

// 必ず target に“放物線で”着地する StartJump
void Enemy::StartJump(const Vector3& start, const Vector3& target,
	float speedXZ /*1フレームあたりの水平速度*/, float /*unused*/)
{
	transform_.translate = start;

	// 水平距離とT(到達フレーム数)を決める
	Vector3 delta = target - start; delta.y = 0.0f;
	float distXZ = MyMath::Length(delta);
	int   T = std::max(1, (int)std::ceil(distXZ / std::max(0.0001f, speedXZ)));

	// 垂直初速：Tフレーム後に y=0 に戻る v0y = -0.5 * g * T
	float v0y = -0.5f * jumpParams_.gravity_ * (float)T;

	// パラメータ保存
	jumpParams_.isJumping_ = true;
	jumpParams_.start_ = start;
	jumpParams_.target_ = target;
	jumpParams_.framesTotal_ = T;
	jumpParams_.framesElapsed_ = 0;
	jumpParams_.v0y_ = v0y;
}

void Enemy::StartDrop(const Vector3& start, const Vector3& target, int frames) {

	transform_.translate = start;

	drop_.active = true;
	drop_.start = start;
	drop_.target = target;
	drop_.framesTotal = std::max(1, frames);
	drop_.framesElapsed = 0;

	// s = (targetY - startY) = v0y*T + 0.5*g*T^2 から v0y を解く
	float s = (target.y - start.y);
	drop_.v0y = (s - 0.5f * drop_.gravity * (float)drop_.framesTotal * (float)drop_.framesTotal)
		/ (float)drop_.framesTotal;
}

// 放物運動の更新（ワープさせない）
void Enemy::Move() {

	// ---- 1) 放物線ジャンプ ----
	if (jumpParams_.isJumping_) {
		// 経過フレームを先に進める
		jumpParams_.framesElapsed_++;
		const int   T = std::max(1, jumpParams_.framesTotal_);
		const int   k = jumpParams_.framesElapsed_;
		const float a = std::min(1.0f, static_cast<float>(k) / static_cast<float>(T)); // 0〜1にサチる

		// 水平は start→target を等速補間（Tフレームで必ず到達）
		Vector3 p = jumpParams_.start_ + (jumpParams_.target_ - jumpParams_.start_) * a;

		// 垂直は等加速度運動: y = v0*t + 0.5*g*t^2
		float y = jumpParams_.v0y_ * static_cast<float>(k)
			+ 0.5f * jumpParams_.gravity_ * static_cast<float>(k) * static_cast<float>(k);

		// 地面(=target.y)より下に行かないようクランプ（固定0ではなく target.y を使う）
		if (y < jumpParams_.target_.y) y = jumpParams_.target_.y;

		transform_.translate = { p.x, y, p.z };

		// ちょうどTフレームで終了（誤差は最終フレームで吸着）
		if (k >= T) {
			transform_.translate = { jumpParams_.target_.x, jumpParams_.target_.y, jumpParams_.target_.z };
			jumpParams_.isJumping_ = false;
		}
		return;
	}

	// ---- 2) 直下降（使っている場合。使っていなければこのブロックごと削除でOK）----
	if (drop_.active) {
		drop_.framesElapsed++;
		const int   T = std::max(1, drop_.framesTotal);
		const int   k = drop_.framesElapsed;
		const float a = std::min(1.0f, static_cast<float>(k) / static_cast<float>(T));

		// XZ は開始→目標（通常同じ）を等速補間
		Vector3 p = drop_.start + (drop_.target - drop_.start) * a;

		// Y は等加速度運動
		float y = drop_.start.y
			+ drop_.v0y * static_cast<float>(k)
			+ 0.5f * drop_.gravity * static_cast<float>(k) * static_cast<float>(k);

		if (y < drop_.target.y) y = drop_.target.y;

		transform_.translate = { p.x, y, p.z };

		if (k >= T) {
			transform_.translate = drop_.target;
			drop_.active = false;
		}
		return;
	}
}

void Enemy::OnHit() {

	isBlinking_ = true;
	blinkCounter_ = 0;
	isAlive_ = false;
}

void Enemy::Attack() {

	if (!player_)
		return; // プレイヤーが未設定なら撃たない

	attackTimer_++;
	if (attackTimer_ < attackInterval_)
		return;
	attackTimer_ = 0;

	auto bulletObject = std::make_unique<Object3d>();
	bulletObject->Init(BlendType::BLEND_NONE);
	bulletObject->SetModel("sphere.obj");
	bulletObject->SetDefaultCamera(camera_);

	auto bullet = std::make_unique<EnemyBullet>();
	bullet->Init(camera_, bulletObject.get());
	bullet->SetTranlate(transform_.translate);

	// プレイヤー座標を使って弾の方向を決定
	Vector3 direction = player_->GetTransform().translate - transform_.translate;
	MyMath::Normalize(direction);
	bullet->SetDirection(direction);

	bulletObjects_.emplace_back(std::move(bullet));
	bulletObject3ds_.emplace_back(std::move(bulletObject));

	// ランダムに次の攻撃間隔を決定（240～300）
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(240, 300);
	attackInterval_ = dist(gen);
}