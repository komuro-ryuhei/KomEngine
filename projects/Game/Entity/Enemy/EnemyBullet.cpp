#include "EnemyBullet.h"
#include "externals/imgui/imgui.h"

float EnemyBullet::GetRadius() const { return radius_; }

void EnemyBullet::Init(Camera* camera, Object3d* object3d) {

	camera_ = camera;
	object3d_ = object3d;

	object3d_->Init(BlendType::BLEND_NONE);
	object3d_->SetDefaultCamera(camera_);

	object3d_->SetScale({0.1f, 0.1f, 0.1f});
}

void EnemyBullet::Update() {

	//
	object3d_->Update();

	transform_.translate += direction_ * speed_;
	object3d_->SetTranslate(transform_.translate);
}

void EnemyBullet::Draw() { object3d_->Draw(); }

void EnemyBullet::ImGuiDebug() {

	//
	ImGui::Begin("PlayerBullet");

	ImGui::DragFloat3("bulletTranlate", &transform_.translate.x, 0.01f);
	ImGui::DragFloat3("bulletRotate", &transform_.rotate.x, 0.01f);

	ImGui::End();
}

Vector3 EnemyBullet::GetTranslate() const { return transform_.translate; }

void EnemyBullet::SetTranlate(Vector3 translate) { transform_.translate = translate; }

void EnemyBullet::SetDirection(const Vector3& direction) { direction_ = direction; }