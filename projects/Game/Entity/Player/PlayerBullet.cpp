#include "PlayerBullet.h"

#ifdef USE_IMGUI

#include "externals/imgui/imgui.h"

#endif

#include "Engine/Base/Particle/ParticleManager.h"

float PlayerBullet::GetRadius() const { return radius_; }

void PlayerBullet::Init(Camera* camera, Object3d* object3d) {

	camera_ = camera;
	object3d_ = object3d;

	object3d_->Init(BlendType::BLEND_NONE);
	object3d_->SetDefaultCamera(camera_);

	object3d_->SetScale({ 0.1f, 0.1f, 0.1f });

	// トレイル用エミッター生成
	trailEmitter_ = std::make_unique<ParticleEmitter>();
	trailEmitter_->Init("trail", transform_.translate, 5); // 1フレームに1個生成くらい
}

void PlayerBullet::Update() {

	// 弾を進める
	transform_.translate += direction_ * speed_;
	object3d_->SetTranslate(transform_.translate);

	// ★ ここを ParticleManager::EmitTrail から Emitter に変更
	if (trailEmitter_) {
		trailEmitter_->SetTranslate(transform_.translate);
		trailEmitter_->Update();   // Update の中で Emit() が呼ばれてパーティクル生成
	}

	// 寿命処理
	lifeTimer_ += 1.0f / 60.0f;
	if (lifeTimer_ >= lifeTime_) {
		isAlive_ = false;
	}

	object3d_->Update();

	if (pendingKill_) {
		isAlive_ = false;
	}
}

void PlayerBullet::Draw() { object3d_->Draw(); }

void PlayerBullet::ImGuiDebug() {

#ifdef USE_IMGUI

	//
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
	// トレイルエミッターの座標も更新
	if (trailEmitter_) {
		trailEmitter_->SetTranslate(translate);
	}
}

void PlayerBullet::SetDirection(const Vector3& direction) { direction_ = direction; }

bool PlayerBullet::IsAlive() const { return isAlive_; }

// ================= ICollisionObject の実装 ================= //

Vector3 PlayerBullet::GetCollisionPosition() const {

	// 弾の中心＝現在のワールド座標
	// Object3d を使ってもいいけど、今は transform を真とする
	return transform_.translate;
}

float PlayerBullet::GetCollisionRadius() const {

// 既存の radius_ をそのまま利用
	return radius_;
}

CollisionLayer PlayerBullet::GetCollisionLayer() const {

	// プレイヤーの弾として扱う
	return CollisionLayer::PlayerBullet;
}

void PlayerBullet::OnCollision(ICollisionObject* other) {

	// 何に当たったかで処理を分ける
	switch (other->GetCollisionLayer()) {

	case CollisionLayer::Enemy:

		isAlive_ = false;
		break;

	case CollisionLayer::EnemyBullet:

		pendingKill_ = true;
		break;

	case CollisionLayer::Environment:

		// 敵やステージに当たったら弾は消える
		isAlive_ = false;
		break;

	default:
		break;
	}
}