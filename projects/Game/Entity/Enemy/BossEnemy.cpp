#include "BossEnemy.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif // USE_IMGUI

#include "Game/Entity/Player/Player.h"
#include "Engine/Base/System/System.h"

void BossEnemy::SetTranslate(Vector3 translate) { transform_.translate = translate; }

void BossEnemy::Init(Camera *camera) {

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

#ifdef USE_IMGUI

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

#endif
}

void BossEnemy::Attack() {

	if (!player_) return;

	switch (attackPhase_) {

		// ================== 左右片手攻撃 ================== //
	case AttackPhase::SingleLeft:
	case AttackPhase::SingleRight:
	{
		const bool useLeft = (attackPhase_ == AttackPhase::SingleLeft);

		Object3d *targetArm = useLeft ? leftArm_.get() : rightArm_.get();
		Vector3 &targetPos = useLeft ? leftArmPos_ : rightArmPos_;
		int &hitCount = useLeft ? leftArmHitCount_ : rightArmHitCount_;

		const Vector3 baseLocalOffset = useLeft
			? Vector3{ -4.0f, 0.0f, 0.0f }
		: Vector3{ 4.0f, 0.0f, 0.0f };

		Vector3 armPos = targetPos;

		const Vector3 worldBase = transform_.translate + baseLocalOffset;
		Vector3 toPlayer = player_->GetTranslate() - worldBase;
		Vector3 dir = MyMath::Normalize(toPlayer);

		if (isExtending_) {
			// 伸ばす
			armPos += dir * attackSpeed_;

			// 一定距離 or 規定ヒット数で戻りフェーズへ
			if (MyMath::Length(armPos - baseLocalOffset) >= 20.0f ||
				hitCount >= maxHitCount_) {
				isExtending_ = false;
			}
		} else {
			// 基本位置へ戻す
			Vector3 toOrigin = baseLocalOffset - armPos;
			float dist = MyMath::Length(toOrigin);

			if (dist < 0.5f) {
				// 戻り完了
				armPos = baseLocalOffset;
				hitCount = 0;
				isExtending_ = true;

				// 次フェーズへ
				if (attackPhase_ == AttackPhase::SingleLeft) {
					attackPhase_ = AttackPhase::SingleRight; // 次は右
				} else {
					attackPhase_ = AttackPhase::BothHands;   // 両手攻撃へ
				}
			} else {
				armPos += MyMath::Normalize(toOrigin) * 0.5f;
			}
		}

		targetArm->SetTranslate(armPos);
		targetPos = armPos;
		break;
	}

	// ================== 両手同時攻撃 ================== //
	case AttackPhase::BothHands:
	{
		const Vector3 leftBase{ -4.0f, 0.0f, 0.0f };
		const Vector3 rightBase{ 4.0f, 0.0f, 0.0f };

		Vector3 leftPos = leftArmPos_;
		Vector3 rightPos = rightArmPos_;

		Vector3 leftWorldBase = transform_.translate + leftBase;
		Vector3 rightWorldBase = transform_.translate + rightBase;

		Vector3 dirL = MyMath::Normalize(player_->GetTranslate() - leftWorldBase);
		Vector3 dirR = MyMath::Normalize(player_->GetTranslate() - rightWorldBase);

		const float maxLen = 18.0f;  // 伸びきる距離
		const float returnSpeed = 0.5f;   // 戻る速度
		const float endThreshold = 0.5f;   // 基準位置に戻ったと判定する距離

		// ===== 左腕 =====
		if (leftExtending_) {
			leftPos += dirL * attackSpeed_;

			bool reachedDist = MyMath::Length(leftPos - leftBase) >= maxLen;
			bool hitEnough = (leftArmHitCount_ >= maxHitCount_);

			// 規定距離 or 規定ヒット数で左腕だけ戻りフェーズへ
			if (reachedDist || hitEnough) {
				leftExtending_ = false;
			}
		} else {
			Vector3 toBase = leftBase - leftPos;
			float dist = MyMath::Length(toBase);

			if (dist < endThreshold) {
				leftPos = leftBase;
			} else {
				leftPos += MyMath::Normalize(toBase) * returnSpeed;
			}
		}

		// ===== 右腕 =====
		if (rightExtending_) {
			rightPos += dirR * attackSpeed_;

			bool reachedDist = MyMath::Length(rightPos - rightBase) >= maxLen;
			bool hitEnough = (rightArmHitCount_ >= maxHitCount_);

			// 規定距離 or 規定ヒット数で右腕だけ戻りフェーズへ
			if (reachedDist || hitEnough) {
				rightExtending_ = false;
			}
		} else {
			Vector3 toBase = rightBase - rightPos;
			float dist = MyMath::Length(toBase);

			if (dist < endThreshold) {
				rightPos = rightBase;
			} else {
				rightPos += MyMath::Normalize(toBase) * returnSpeed;
			}
		}

		// 位置を反映
		if (leftArm_)  leftArm_->SetTranslate(leftPos);
		if (rightArm_) rightArm_->SetTranslate(rightPos);
		leftArmPos_ = leftPos;
		rightArmPos_ = rightPos;

		// ===== メテオ移行判定 =====
		bool leftFinished = !leftExtending_ && MyMath::Length(leftPos - leftBase) < endThreshold;
		bool rightFinished = !rightExtending_ && MyMath::Length(rightPos - rightBase) < endThreshold;

		// 両方「戻り完了」したらメテオへ
		if (leftFinished && rightFinished) {
			leftArmHitCount_ = 0;
			rightArmHitCount_ = 0;

			leftExtending_ = true;
			rightExtending_ = true;

			attackPhase_ = AttackPhase::WaitMeteor;
			meteorRequest_ = true;
		}

		break;
	}

	// ================== メテオ待ち ================== //
	case AttackPhase::WaitMeteor:
		// Scene側でメテオを出している間は腕攻撃しない
		break;
	}
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

bool BossEnemy::ConsumeMeteorRequest() {

	if (meteorRequest_) {
		meteorRequest_ = false;
		return true;
	}
	return false;
}

void BossEnemy::OnMeteorFinished() {

	// 次は左片手攻撃から再開
	attackPhase_ = AttackPhase::SingleLeft;
	isExtending_ = true;

	// 片手フェーズ用にヒット数リセット
	leftArmHitCount_ = 0;
	rightArmHitCount_ = 0;

	// 両手フェーズ用フラグも初期化
	leftExtending_ = true;
	rightExtending_ = true;

	// 腕位置を基準に戻しておく（お好みで）
	leftArmPos_ = { -4.0f, 0.0f, 0.0f };
	rightArmPos_ = { 4.0f, 0.0f, 0.0f };
	if (leftArm_)  leftArm_->SetTranslate(leftArmPos_);
	if (rightArm_) rightArm_->SetTranslate(rightArmPos_);
}

void BossEnemy::SetRotate(Vector3 &rotate) {
	transform_.rotate = rotate;
	object3d_->SetRotate(rotate);
	leftArm_->SetRotate(rotate);
	rightArm_->SetRotate(rotate);
}

// 末尾あたりに実装を追加

void BossEnemy::SetRightHandScale(const Vector3 &s) {
	if (rightArm_) {
		rightArm_->SetScale(s);
		// 半径は Scale に応じて毎フレーム Update で設定しているが、
		// 念のためここでも更新しておくと安全
		rightArm_->SetRadius(1.0f * rightArm_->GetScale().x);
	}
}

Vector3 BossEnemy::GetRightHandWorldPos() const {
	if (rightArm_) {
		// 子オブジェクトなので WorldPosition を取るのが正確
		return rightArm_->GetWorldPosition();
	}
	// フォールバック（親＋ローカル）
	return transform_.translate + rightArmPos_;
}

float BossEnemy::GetRightHandRadius() const {
	if (rightArm_) {
		// Update() で「1.0f * scale.x」を SetRadius 済み
		return rightArm_->GetRadius();
	}
	// フォールバック（右手の基準半径=1.0f）
	return 1.0f;
}

// ---- 左手（必要なら使って） ----
void BossEnemy::SetLeftHandScale(const Vector3 &s) {

	if (leftArm_) {
		leftArm_->SetScale(s);
		leftArm_->SetRadius(1.0f * leftArm_->GetScale().x);
	}
}

Vector3 BossEnemy::GetLeftHandWorldPos() const {

	if (leftArm_) {
		return leftArm_->GetWorldPosition();
	}
	return transform_.translate + leftArmPos_;
}

float BossEnemy::GetLeftHandRadius() const {

	if (leftArm_) {
		return leftArm_->GetRadius();
	}
	return 1.0f;
}

void BossEnemy::Damage(int v) {

	if (v <= 0) return;
	hp_ = std::max(0, hp_ - v);
}

Vector3 BossEnemy::GetCurrentArmWorldPos() const {

	switch (attackPhase_) {
	case AttackPhase::SingleLeft:
		if (leftArm_)  return leftArm_->GetWorldPosition();
		return transform_.translate + leftArmPos_;

	case AttackPhase::SingleRight:
		if (rightArm_) return rightArm_->GetWorldPosition();
		return transform_.translate + rightArmPos_;

	case AttackPhase::BothHands:
		// 両手攻撃中は真ん中あたり返しておく（レティクル用）
		if (leftArm_ && rightArm_) {
			return (leftArm_->GetWorldPosition() + rightArm_->GetWorldPosition()) * 0.5f;
		}
		break;

	case AttackPhase::WaitMeteor:
	default:
		break;
	}

	// フォールバック（左腕基準）
	if (leftArm_) return leftArm_->GetWorldPosition();
	return transform_.translate + leftArmPos_;
}