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
	leftArmPos_ = { -4.0f, 0.0f, 0.0f };
	leftArmRot_ = { 0.0f, 3.0f, 0.0f };

	rightArm_ = std::make_unique<Object3d>();
	rightArm_->Init(BlendType::BLEND_NONE);
	rightArm_->SetModel("hand.obj");
	rightArm_->SetDefaultCamera(camera_);
	rightArm_->SetParent(object3d_.get());
	rightArm_->SetScale({ 1.0f, 1.0f, 1.0f });
	rightArm_->SetTranslate({ 4.0f, 0.0f, 0.0f });
	rightArmPos_ = { 4.0f, 0.0f, 0.0f };
}

void BossEnemy::Update() {

	object3d_->Update();
	leftArm_->Update();
	rightArm_->Update();

	object3d_->SetTranslate(transform_.translate);
	object3d_->SetRotate(transform_.rotate);

	rightArm_->SetTranslate(rightArmPos_);
	leftArm_->SetTranslate(leftArmPos_);
	rightArm_->SetRotate(rightArmRot_);
	leftArm_->SetRotate(leftArmRot_);

	// radius（スケールベース）を設定
	object3d_->SetRadius(2.0f * object3d_->GetScale().x);
	leftArm_->SetRadius(1.0f * leftArm_->GetScale().x);
	rightArm_->SetRadius(1.0f * rightArm_->GetScale().x);

	if (System::GetInput()->PushKey(DIK_SPACE)) {
		pushEnter_ = true;
	}

	// 攻撃フラグが立っていたら攻撃
	if (isAttack_) {
		Attack();
	}

	// TitleScene用の動き
	if (pushEnter_) {
		if (isInTitleScene_) {
			TitleSceneMove();
		}
	}
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

	ImGui::DragInt("R_HitCount", &rightArmHitCount_);
	ImGui::DragInt("L_HitCount", &leftArmHitCount_);

	ImGui::DragFloat3("rightArmPos", &rightArmPos_.x, 0.01f);
	ImGui::DragFloat3("rightArmRot", &rightArmRot_.x, 0.01f);
	ImGui::DragFloat3("leftArmPos", &leftArmPos_.x, 0.01f);
	ImGui::DragFloat3("leftArmRot", &leftArmRot_.x, 0.01f);

	ImGui::Checkbox("isAttack", &isAttack_);
	ImGui::End();
}

void BossEnemy::Attack() {

	if (!player_) return;

	// どっちの腕で攻撃するか決定
	Object3d* targetArm = attackLeftArm_ ? leftArm_.get() : rightArm_.get();
	Vector3& targetPos = attackLeftArm_ ? leftArmPos_ : rightArmPos_;
	int& hitCount = attackLeftArm_ ? leftArmHitCount_ : rightArmHitCount_;

	// 腕のローカル基準位置（初期の取り付け位置）
	const Vector3 baseLocalOffset = attackLeftArm_ ? Vector3{ -4.0f, 0.0f, 0.0f }
	: Vector3{ 4.0f, 0.0f, 0.0f };

	// 現在の腕ローカル位置（※必ず“その腕”の位置から始める）
	Vector3 armPos = targetPos;

	// プレイヤーへの方向（ワールド→ローカル混在を避けたいなら将来は親回転を考慮）
	const Vector3 worldBase = transform_.translate + baseLocalOffset;
	Vector3 toPlayer = player_->GetTranslate() - worldBase;
	Vector3 dir = MyMath::Normalize(toPlayer);

	// 伸縮ステート
	if (isExtending_) {
		// 伸ばす
		armPos += dir * attackSpeed_;

		// 到達 or 規定回数ヒットで引き戻しへ
		if (MyMath::Length(armPos - baseLocalOffset) >= 20.0f || hitCount >= maxHitCount_) {
			isExtending_ = false;
		}
	} else {
		// 基準位置へ戻す
		Vector3 toOrigin = baseLocalOffset - armPos;
		float dist = MyMath::Length(toOrigin);
		if (dist < 0.5f) {
			// 完全に戻ったら次の腕へ
			armPos = baseLocalOffset;
			isExtending_ = true;
			isAttacking_ = false;
			attackLeftArm_ = !attackLeftArm_;
			hitCount = 0;  // カウントリセット
		} else {
			armPos += MyMath::Normalize(toOrigin) * 0.5f;
		}
	}

	// モデルに反映 & ローカル保存（次フレームで消えないように）
	targetArm->SetTranslate(armPos);
	targetPos = armPos;
}

void BossEnemy::TitleSceneMove() {

	if (!isMoveRight_) {
		if (leftArmPos_.x >= 0.19f) {
			leftArmPos_.x -= 0.1f;
		} else if (leftArmPos_.x <= 0.19f) {
			isMoveRight_ = true;
		}
	}
	// 
	if (isMoveRight_) {
		if (leftArmPos_.x <= 5.8f) {
			leftArmPos_.x += 0.1f;
		}
	}
}

void BossEnemy::AddHitToAttackingArm() {

	if (attackLeftArm_) {
		++leftArmHitCount_;
	} else {
		++rightArmHitCount_;
	}
}

void BossEnemy::InitTitleScenePos() {

	// 
	rightArmPos_ = { -0.15f,0.0f,-12.0f };
	rightArmRot_ = { 0.0f,-1.57f,0.0f };
	leftArmPos_ = { 0.19f,0.0f,-12.0f };
	// leftArmPos_ = { 5.9f,0.0f,-12.0f };
	leftArmRot_ = { 0.0f,1.56f,0.0f };
}

void BossEnemy::SetRotate(Vector3& rotate) {
	transform_.rotate = rotate;
	object3d_->SetRotate(rotate);
	leftArm_->SetRotate(rotate);
	rightArm_->SetRotate(rotate);
}