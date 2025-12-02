#include "BossEnemy.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif // USE_IMGUI

#include "Game/Entity/Player/Player.h"
#include "Engine/Base/System/System.h"
#include "Engine/Base/Particle/ParticleManager.h"

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

	// 
	hpSprite_ = std::make_unique<Sprite>();
	hpSprite_->Init("./Resources/images/hp.png", BlendType::BLEND_NONE);
	hpSprite_->SetAnchorPoint({ 0.0f, 0.5f });
	hpSprite_->SetSize({ 700.0f,50.0f });
	hpSprite_->SetPosition({ 200.0f,100.0f });
}

void BossEnemy::Update() {

	// 3Dオブジェクト更新
	object3d_->Update();
	leftArm_->Update();
	rightArm_->Update();

	// HPバーの更新
	if (hpSprite_) {
		float hpRatio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
		hpRatio = std::clamp(hpRatio, 0.0f, 1.0f);
		Vector2 baseSize = { 700.0f, 50.0f };
		hpSprite_->SetSize({ baseSize.x * hpRatio, baseSize.y });
		hpSprite_->Update();
	}

	// ---------------------- 撃破後 / 生存中で分岐 ---------------------- //

	if (hp_ <= 0) {

		if (!fallStarted_) {
			fallStarted_ = true;
			fallVelY_ = 0.0f;

			// ★開始姿勢の保存
			fallRotateStart_ = transform_.rotate.x;

			// ★重力を弱くする（ゆっくり落下）
			// gravityY_ = -0.01f;

			// シェイクタイマー初期化
			fallShakeTime_ = 0.0f;
		}

		if (!hasLanded_) {

			fallShakeTime_ += 1.0f / 60.0f;

			// ---- 落下 ----
			fallVelY_ += gravityY_;
			transform_.translate.y += fallVelY_;

			// ---- 回転（前に倒れる）----
			//   落下の進行度で角度をなめらかに変化
			float fallProgress = (transform_.translate.y - groundY_) / (2.0f - groundY_);
			fallProgress = std::clamp(1.0f - fallProgress, 0.0f, 1.0f);

			// イージング（顔から落ちる時ちょっと速くする）
			float ease = fallProgress * fallProgress;

			transform_.rotate.x = MyMath::Lerp(fallRotateStart_, fallRotateEnd_, ease);

			// ---- 地面に到達したら停止 ----
			if (transform_.translate.y <= groundY_) {
				transform_.translate.y = groundY_;
				fallVelY_ = 0.0f;
				hasLanded_ = true;

				// ★ ここで一度だけカメラシェイク
				if (!landingShakeDone_ && camera_) {
					camera_->StartShake(CameraShakeType::Large);
					landingShakeDone_ = true;
				}
				if (transform_.translate.y <= groundY_) {
					transform_.translate.y = groundY_;
					fallVelY_ = 0.0f;
					hasLanded_ = true;

					// ★ 着地時シェイク（既存）
					if (!landingShakeDone_ && camera_) {
						camera_->StartShake(CameraShakeType::Large);
						landingShakeDone_ = true;
					}
					// ★ 着地時に砂ぼこりパーティクル発生
					ParticleManager::GetInstance()->Emit("dust", transform_.translate, 80);
				}

			}
		}
	} else {
		// ★ 生きている間の従来処理

		if (System::GetInput()->PushKey(DIK_SPACE)) {
			pushEnter_ = true;
		}

		// 攻撃
		if (isAttack_) {
			Attack();
		}

		// TitleScene用移動
		if (pushEnter_) {
			if (isInTitleScene_) {
				TitleSceneMove();
			}
		}
	}

	// ---------------------- SRT 反映＆当たり判定用半径 ---------------------- //

	// ★ 落下中だけ見た目にシェイクをかける
	Vector3 drawPos = transform_.translate;
	if (hp_ <= 0 && !hasLanded_) {
		// ちょっと不規則な揺れにするため周波数を変えたsin/cosを足す
		float sx = std::sin(fallShakeTime_ * 40.0f) * fallShakeAmplitude_;
		float sz = std::cos(fallShakeTime_ * 55.0f) * fallShakeAmplitude_;
		drawPos.x += sx;
		drawPos.z += sz;
	}

	object3d_->SetTranslate(drawPos);
	object3d_->SetRotate(transform_.rotate);

	rightArm_->SetTranslate(rightArmPos_);
	leftArm_->SetTranslate(leftArmPos_);
	rightArm_->SetRotate(rightArmRot_);
	leftArm_->SetRotate(leftArmRot_);


	object3d_->SetRadius(1.5f * object3d_->GetScale().x);
	leftArm_->SetRadius(1.0f * leftArm_->GetScale().x);
	rightArm_->SetRadius(1.0f * rightArm_->GetScale().x);
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

	// object3d_->ImGuiDebug("Boss");

	ImGui::DragInt("R_HitCount", &rightArmHitCount_);
	ImGui::DragInt("L_HitCount", &leftArmHitCount_);

	ImGui::DragFloat3("rightArmPos", &rightArmPos_.x, 0.01f);
	ImGui::DragFloat3("rightArmRot", &rightArmRot_.x, 0.01f);
	ImGui::DragFloat3("leftArmPos", &leftArmPos_.x, 0.01f);
	ImGui::DragFloat3("leftArmRot", &leftArmRot_.x, 0.01f);

	ImGui::DragInt("HP", &hp_);

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

		Object3d* targetArm = useLeft ? leftArm_.get() : rightArm_.get();
		Vector3& targetPos = useLeft ? leftArmPos_ : rightArmPos_;
		int& hitCount = useLeft ? leftArmHitCount_ : rightArmHitCount_;

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
		// ローカル基準位置
		const Vector3 leftBaseLocal{ -4.0f, 0.0f, 0.0f };
		const Vector3 rightBaseLocal{ 4.0f, 0.0f, 0.0f };

		// ワールド基準位置
		const Vector3 leftBaseWorld = transform_.translate + leftBaseLocal;
		const Vector3 rightBaseWorld = transform_.translate + rightBaseLocal;

		// 今のワールド位置（毎フレーム取得）
		Vector3 leftWorldPos = leftArm_->GetWorldPosition();
		Vector3 rightWorldPos = rightArm_->GetWorldPosition();

		const Vector3 playerPos = player_->GetTranslate();

		const float maxLen = 22.0f;    // どこまで伸ばすか（必要なら調整）
		const float extendSpeed = attackSpeed_;
		const float returnSpeed = 0.6f;
		const float endThreshold = 0.3f;

		// ===== 左腕 =====
		if (leftExtending_) {
			Vector3 dirL = MyMath::Normalize(playerPos - leftBaseWorld);
			leftWorldPos += dirL * extendSpeed;

			float len = MyMath::Length(leftWorldPos - leftBaseWorld);
			bool reachedDist = (len >= maxLen);
			bool hitEnough = (leftArmHitCount_ >= maxHitCount_);

			if (reachedDist || hitEnough) {
				leftExtending_ = false;
			}
		} else {
			Vector3 toBase = leftBaseWorld - leftWorldPos;
			float dist = MyMath::Length(toBase);
			if (dist < endThreshold) {
				leftWorldPos = leftBaseWorld;
			} else {
				leftWorldPos += MyMath::Normalize(toBase) * returnSpeed;
			}
		}

		// ===== 右腕 =====
		if (rightExtending_) {
			Vector3 dirR = MyMath::Normalize(playerPos - rightBaseWorld);
			rightWorldPos += dirR * extendSpeed;

			float len = MyMath::Length(rightWorldPos - rightBaseWorld);
			bool reachedDist = (len >= maxLen);
			bool hitEnough = (rightArmHitCount_ >= maxHitCount_);

			if (reachedDist || hitEnough) {
				rightExtending_ = false;
			}
		} else {
			Vector3 toBase = rightBaseWorld - rightWorldPos;
			float dist = MyMath::Length(toBase);
			if (dist < endThreshold) {
				rightWorldPos = rightBaseWorld;
			} else {
				rightWorldPos += MyMath::Normalize(toBase) * returnSpeed;
			}
		}

		// ===== ワールド→ローカルに戻して反映 =====
		Vector3 leftLocal = leftWorldPos - transform_.translate;
		Vector3 rightLocal = rightWorldPos - transform_.translate;

		leftArm_->SetTranslate(leftLocal);
		rightArm_->SetTranslate(rightLocal);
		leftArmPos_ = leftLocal;
		rightArmPos_ = rightLocal;

		// ===== メテオ移行判定 =====
		bool leftFinished =
			!leftExtending_ &&
			MyMath::Length(leftWorldPos - leftBaseWorld) < endThreshold;

		bool rightFinished =
			!rightExtending_ &&
			MyMath::Length(rightWorldPos - rightBaseWorld) < endThreshold;

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

void BossEnemy::HPDraw() {

	// 
	hpSprite_->Draw();
}

void BossEnemy::SetRotate(Vector3& rotate) {
	transform_.rotate = rotate;
	object3d_->SetRotate(rotate);
	leftArm_->SetRotate(rotate);
	rightArm_->SetRotate(rotate);
}

// 末尾あたりに実装を追加

void BossEnemy::SetRightHandScale(const Vector3& s) {
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
void BossEnemy::SetLeftHandScale(const Vector3& s) {

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