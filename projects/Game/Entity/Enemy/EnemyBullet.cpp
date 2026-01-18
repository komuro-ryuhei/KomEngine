#include "EnemyBullet.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

float EnemyBullet::GetRadius() const { return radius_; }

void EnemyBullet::Init(Camera* camera, Object3d* object3d) {

	camera_ = camera;
	object3d_ = object3d;

	object3d_->Init(BlendType::BLEND_NONE);
	object3d_->SetDefaultCamera(camera_);

	isDead_ = false;
	hitPlayer_ = false;

	transform_.translate = object3d_->GetTranslate();
	object3d_->SetTranslate(transform_.translate);

}

void EnemyBullet::Update() {

	if (isDead_) { return; }

	//
	object3d_->Update();

	transform_.translate += direction_ * speed_;
	object3d_->SetTranslate(transform_.translate);
}

void EnemyBullet::Draw() {

	if (isDead_) { return; }
	object3d_->Draw();
}

void EnemyBullet::ImGuiDebug() {

#ifdef USE_IMGUI

	//
	ImGui::Begin("EnemyBullet");

	ImGui::DragFloat3("bulletTranlate", &transform_.translate.x, 0.01f);
	ImGui::DragFloat3("bulletRotate", &transform_.rotate.x, 0.01f);

	ImGui::End();

#endif
}

void EnemyBullet::OnCollision(ICollisionObject* other) {

	if (isDead_) return;
	if (!other) { return; }

	const auto layer = other->GetCollisionLayer();

	// Player or PlayerBullet に当たったら消す
	if (layer == CollisionLayer::Player) {
		hitPlayer_ = true;
		if (destroyOnPlayerHit_) {
			isDead_ = true;
		}
	} else if (layer == CollisionLayer::PlayerBullet) {
		isDead_ = true;
	}
}

Vector3 EnemyBullet::GetTranslate() const { return transform_.translate; }

void EnemyBullet::SetTranlate(Vector3 translate) {
	transform_.translate = translate;
	if (object3d_) {
		object3d_->SetTranslate(translate);
	}
}

void EnemyBullet::SetDirection(const Vector3& direction) { direction_ = direction; }
void EnemyBullet::SetSpeed(float speed) { speed_ = speed; }