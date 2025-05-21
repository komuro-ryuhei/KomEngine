
#include "PlayerBullet.h"

#include "externals/imgui/imgui.h"

float PlayerBullet::GetRadius() const { return radius_; }

void PlayerBullet::Init(Camera* camera, Object3d* object3d) {

	camera_ = camera;
	object3d_ = object3d;

	object3d_->Init(BlendType::BLEND_NONE);
	object3d_->SetDefaultCamera(camera_);

	object3d_->SetScale({0.1f, 0.1f, 0.1f});
}

void PlayerBullet::Update() {

	// 
	object3d_->Update();

	transform_.translate += direction_ * speed_;
	object3d_->SetTranslate(transform_.translate);
}

void PlayerBullet::Draw() { object3d_->Draw(); }

void PlayerBullet::ImGuiDebug() {

	//
	ImGui::Begin("PlayerBullet");

	ImGui::DragFloat3("bulletTranlate", &transform_.translate.x, 0.01f);
	ImGui::DragFloat3("bulletRotate", &transform_.rotate.x, 0.01f);

	ImGui::End();
}

Vector3 PlayerBullet::GetTranslate() const { return transform_.translate; }

void PlayerBullet::SetTranlate(Vector3 translate) { transform_.translate = translate; }

void PlayerBullet::SetDirection(const Vector3& direction) { direction_ = direction; }