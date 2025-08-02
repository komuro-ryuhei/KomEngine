#include "BossEnemy.h"
#include "externals/imgui/imgui.h"
#include "Game/Entity/Player/Player.h"
#include "Engine/Base/System/System.h"

void BossEnemy::SetTranslate(Vector3 translate) { transform_.translate = translate; }

void BossEnemy::Init(Camera* camera) {

	// カメラの設定
	camera_ = camera;

	// 自機オブジェクトの生成
	object3d_ = std::make_unique<Object3d>();
	object3d_->Init(BlendType::BLEND_NONE);

	object3d_->SetModel("sphere.obj");
	object3d_->SetDefaultCamera(camera_);
	object3d_->SetScale({ 2.0f, 2.0f, 2.0f });

	leftArm_ = std::make_unique<Object3d>();
	leftArm_->Init(BlendType::BLEND_NONE);
	leftArm_->SetModel("sphere.obj");
	leftArm_->SetDefaultCamera(camera_);
	leftArm_->SetParent(object3d_.get());
	leftArm_->SetScale({ 1.0f, 1.0f, 1.0f });
	leftArm_->SetTranslate({ -4.0f, 0.0f, 0.0f });

	rightArm_ = std::make_unique<Object3d>();
	rightArm_->Init(BlendType::BLEND_NONE);
	rightArm_->SetModel("sphere.obj");
	rightArm_->SetDefaultCamera(camera_);
	rightArm_->SetParent(object3d_.get());
	rightArm_->SetScale({ 1.0f, 1.0f, 1.0f });
	rightArm_->SetTranslate({ 4.0f, 0.0f, 0.0f });
}

void BossEnemy::Update() {

	object3d_->Update();
	leftArm_->Update();
	rightArm_->Update();

	object3d_->SetTranslate(transform_.translate);
	object3d_->SetRotate(transform_.rotate);

	// radius（スケールベース）を設定
	object3d_->SetRadius(2.0f * object3d_->GetScale().x);
	leftArm_->SetRadius(1.0f * leftArm_->GetScale().x);
	rightArm_->SetRadius(1.0f * rightArm_->GetScale().x);

	Attack();
}


void BossEnemy::Draw() {

	//
	object3d_->Draw();
	leftArm_->Draw();
	rightArm_->Draw();
}

void BossEnemy::ImGuiDebug() {

	// 

	ImGui::Begin("BossEnemy");

	object3d_->ImGuiDebug();

	ImGui::End();
}

void BossEnemy::Attack() {

	if (!player_) return;

	// クールダウンが終わってないなら何もしない
	if (!isAttacking_) {
		attackCooldown_ += 1.0f / 60.0f; // フレーム更新
		if (attackCooldown_ >= attackInterval_) {
			isAttacking_ = true;
			attackCooldown_ = 0.0f;
		} else {
			return; // 待機中
		}
	}

	// 攻撃中
	Vector3 baseArmPos = { -4.0f, 0.0f, 0.0f };
	Vector3 playerPos = player_->GetTranslate();
	Vector3 target = playerPos - (transform_.translate + baseArmPos);
	Vector3 direction = MyMath::Normalize(target);
	Vector3 armPos = leftArm_->GetTranslate();

	if (isExtending_) {
		armPos += direction * 0.5f;
		if (MyMath::Length(armPos - baseArmPos) >= 20.0f) {
			isExtending_ = false;
		}
	} else {
		// 元に戻す処理
		Vector3 toOrigin = baseArmPos - armPos;
		if (MyMath::Length(toOrigin) < 0.5f) {
			armPos = baseArmPos;
			isExtending_ = true;
			isAttacking_ = false; // 攻撃終了 → 次の待機へ
		} else {
			armPos += MyMath::Normalize(toOrigin) * 0.5f;
		}
	}

	leftArm_->SetTranslate(armPos);
}