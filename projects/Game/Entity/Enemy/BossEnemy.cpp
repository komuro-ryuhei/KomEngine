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

	object3d_->SetModel("BossEnemy.obj");
	object3d_->SetDefaultCamera(camera_);
	object3d_->SetScale({ 2.0f, 2.0f, 2.0f });

	leftArm_ = std::make_unique<Object3d>();
	leftArm_->Init(BlendType::BLEND_NONE);
	leftArm_->SetModel("hand.obj");
	leftArm_->SetDefaultCamera(camera_);
	leftArm_->SetParent(object3d_.get());
	leftArm_->SetScale({ 1.0f, 1.0f, 1.0f });
	leftArm_->SetTranslate({ -4.0f, 0.0f, 0.0f });
	leftArm_->SetRotate({ 0.0f, 3.0f, 0.0f });

	rightArm_ = std::make_unique<Object3d>();
	rightArm_->Init(BlendType::BLEND_NONE);
	rightArm_->SetModel("hand.obj");
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

	Object3d* targetArm = attackLeftArm_ ? leftArm_.get() : rightArm_.get();
	Vector3 baseLocalOffset = attackLeftArm_ ? Vector3{ -4.0f, 0, 0 } : Vector3{ 4.0f, 0, 0 };
	Vector3 armPos = targetArm->GetTranslate(); // ローカル座標

	Vector3 worldBase = transform_.translate + baseLocalOffset;
	Vector3 target = player_->GetTranslate() - worldBase;
	Vector3 direction = MyMath::Normalize(target);

	int& hitCount = attackLeftArm_ ? leftArmHitCount_ : rightArmHitCount_;

	if (isExtending_) {
		armPos += direction * attackSpeed_;

		// 条件1: ある程度伸びたら戻す
		// 条件2: ヒットカウントが上限に達したら戻す
		if (MyMath::Length(armPos - baseLocalOffset) >= 20.0f || hitCount >= maxHitCount_) {
			isExtending_ = false;
		}

	} else {
		// 元に戻る処理
		Vector3 toOrigin = baseLocalOffset - armPos;
		if (MyMath::Length(toOrigin) < 0.5f) {
			armPos = baseLocalOffset;
			isExtending_ = true;
			isAttacking_ = false;
			attackLeftArm_ = !attackLeftArm_;
			hitCount = 0;  // カウントリセット
		} else {
			armPos += MyMath::Normalize(toOrigin) * 0.5f;
		}
	}

	targetArm->SetTranslate(armPos);
}

void BossEnemy::AddHitToAttackingArm() {
	if (attackLeftArm_) {
		++leftArmHitCount_;
	} else {
		++rightArmHitCount_;
	}
}

void BossEnemy::SetRotate(Vector3& rotate) {
	transform_.rotate = rotate;
	object3d_->SetRotate(rotate);
	leftArm_->SetRotate(rotate);
	rightArm_->SetRotate(rotate);
}