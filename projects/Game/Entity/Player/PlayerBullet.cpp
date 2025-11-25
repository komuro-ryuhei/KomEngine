#include "PlayerBullet.h"

#ifdef USE_IMGUI

#include "externals/imgui/imgui.h"

#endif

float PlayerBullet::GetRadius() const { return radius_; }

void PlayerBullet::Init(Camera *camera, Object3d *object3d) {

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

void PlayerBullet::SetDirection(const Vector3 &direction) { direction_ = direction; }

bool PlayerBullet::IsAlive() const { return isAlive_; }