#include "BossEnemy.h"

void BossEnemy::Init(Camera* camera) {

	camera_ = camera;

	// 自機オブジェクトの生成
	object3d_ = std::make_unique<Object3d>();
	object3d_->Init(BlendType::BLEND_NONE);

	object3d_->SetModel("sphere.obj");
	object3d_->SetDefaultCamera(camera_);

	object3d_->SetScale({ 2.0f, 2.0f, 2.0f });
}

void BossEnemy::Update() {

	object3d_->Update();

	object3d_->SetTranslate(transform_.translate);
	object3d_->SetRotate(transform_.rotate);
}

void BossEnemy::Draw() {

	//
	object3d_->Draw();
}