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

// 放物運動の更新（ワープさせない）
void Enemy::Move() {
	if (!jumpParams_.isJumping_) return;

	// 経過フレームを進める
	jumpParams_.framesElapsed_++;
	int   k = jumpParams_.framesElapsed_;
	int   T = jumpParams_.framesTotal_;
	float a = (float)k / (float)T;                 // 0→1

	// 水平は start→target を等速で補間（Tフレームで必ず到達）
	Vector3 p = jumpParams_.start_ + (jumpParams_.target_ - jumpParams_.start_) * a;
	p.y = 0.0f;

	// 垂直は等加速度運動: y = v0*t + 0.5*g*t^2
	float y = jumpParams_.v0y_ * (float)k + 0.5f * jumpParams_.gravity_ * (float)k * (float)k;
	if (y < 0.0f) y = 0.0f;                         // 数値誤差のクランプ

	transform_.translate = { p.x, y, p.z };

	// ちょうどTフレームで終了（誤差は最終フレームで吸着）
	if (k >= T) {
		transform_.translate = { jumpParams_.target_.x, 0.0f, jumpParams_.target_.z };
		jumpParams_.isJumping_ = false;
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