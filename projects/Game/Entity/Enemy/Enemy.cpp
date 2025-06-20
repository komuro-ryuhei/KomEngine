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
std::vector<std::unique_ptr<EnemyBullet>>& Enemy::GetBullets() { return bulletObjects_; }

void Enemy::SetPlayer(Player* player) { player_ = player; }

void Enemy::Init(Camera* camera, Object3d* object3d) {

	camera_ = camera;
	object3d_ = object3d;

	object3d_->Init(BlendType::BLEND_NONE);
}

void Enemy::Update() {

	if (isBlinking_) {
		blinkCounter_++;
		if (blinkCounter_ >= blinkDuration_) {
			isBlinking_ = false;
			blinkCounter_ = 0;
		}
	}

	// 攻撃処理
	Attack();

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