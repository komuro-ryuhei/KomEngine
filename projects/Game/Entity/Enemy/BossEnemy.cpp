#include "BossEnemy.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif // USE_IMGUI

#include "Game/Entity/Player/Player.h"
#include "Game/Entity/Player/PlayerBullet.h"
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

	baseBodyScale_ = { 2.0f, 2.0f, 2.0f };
	baseArmScale_ = { 1.0f, 1.0f, 1.0f };

	// 
	hpSprite_ = std::make_unique<Sprite>();
	hpSprite_->Init("./Resources/images/hp.png", BlendType::BLEND_NONE);
	hpSprite_->SetAnchorPoint({ 0.0f, 0.5f });
	hpSprite_->SetSize({ 700.0f,50.0f });
	hpSprite_->SetPosition({ 200.0f,100.0f });

	bodyCol_.owner = this;
	bodyCol_.part = PartCollider::Part::Body;

	leftCol_.owner = this;
	leftCol_.part = PartCollider::Part::LeftArm;

	rightCol_.owner = this;
	rightCol_.part = PartCollider::Part::RightArm;

	// --- 怒り用：通常時の基準値を保存 ---
	baseAttackSpeed_ = attackSpeed_;
	baseArmReturnSpeedSingle_ = armReturnSpeedSingle_;
	baseArmReturnSpeedBoth_ = armReturnSpeedBoth_;
}

void BossEnemy::Update() {

	const float dt = System::GetDeltaTime();

	UpdateChargeCrossPose(dt);

	ChargeEffect(dt);

	// 3Dオブジェクト更新
	object3d_->Update();
	leftArm_->Update();
	rightArm_->Update();

	// 被弾シェイクタイマー
	auto updateShake = [dt](float& t) {
		if (t > 0.0f) {
			t -= dt;
			if (t < 0.0f) t = 0.0f;
		}
		};
	updateShake(bodyHitShakeTime_);
	updateShake(leftHitShakeTime_);
	updateShake(rightHitShakeTime_);

	// 被弾フラッシュタイマー
	auto updateFlash = [dt](float& t) {
		if (t > 0.0f) {
			t -= dt;
			if (t < 0.0f) t = 0.0f;
		}
		};
	updateFlash(bodyHitFlashTime_);
	updateFlash(leftHitFlashTime_);
	updateFlash(rightHitFlashTime_);

	// ---------------------------- HPバーの更新更新 ---------------------------- //

	if (hpSprite_) {
		float hpRatio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
		hpRatio = std::clamp(hpRatio, 0.0f, 1.0f);
		Vector2 baseSize = { 700.0f, 50.0f };
		hpSprite_->SetSize({ baseSize.x * hpRatio, baseSize.y });
		hpSprite_->Update();
	}

	// ---------------------------- HPチップの更新 ---------------------------- //

	const float gravity = 900.0f;   // 下方向加速度(px/s^2) 好きに調整

	for (auto it = hpChips_.begin(); it != hpChips_.end();) {

		it->life -= dt;
		if (it->life <= 0.0f) {
			it = hpChips_.erase(it);
			continue;
		}

		// 重力
		it->vel.y += gravity * dt;

		// 位置更新
		it->pos.x += it->vel.x * dt;
		it->pos.y += it->vel.y * dt;

		if (it->sprite) {
			it->sprite->SetPosition(it->pos);
			it->sprite->Update();
		}

		++it;
	}

	// ---------------------- 撃破後 / 生存中で分岐 ---------------------- //

	if (hp_ <= 0) {

		if (!fallStarted_) {
			StopAllAttacksOnDeath();
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

			fallShakeTime_ += dt;

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
					// 撃破後の着地時に砂ぼこりパーティクル発生
					ParticleManager::GetInstance()->Emit("dust", transform_.translate, 80);
				}

			}
		}
	} else {
		// 生きている間の従来処理

		if (System::GetInput()->PushKey(DIK_SPACE)) {
			pushEnter_ = true;
		}

		// 退避中は Boss 自身が動くので先に更新
		UpdateRetreat(dt);

		// 退避中は腕攻撃は止める（奥で別攻撃する想定）
		if (IsRetreating()) {
			// 退避中は通常攻撃をしない
		} else {
			if (combatEnabled_ && isAttack_ && !chargeActive_) {
				Attack();
			}
		}

		// TitleScene用移動
		if (pushEnter_) {
			if (isInTitleScene_) {
				TitleSceneMove();
			}
		}
	}

	if (hp_ > 0) {
		UpdateMissileVolley(dt);
		UpdateChargeBeamShot(dt);
	}

	// ---------------------- 被弾シェイク ---------------------- //
	DamageShake();

	// 胴体はBodyRadiusを使う
	object3d_->SetRadius(bodyRadius_);

	// 腕はそれぞれ専用の半径を使う
	leftArm_->SetRadius(leftArmRadius_ * leftArm_->GetScale().x);
	rightArm_->SetRadius(rightArmRadius_ * rightArm_->GetScale().x);
}

void BossEnemy::Draw() {

	//
	object3d_->Draw();
	leftArm_->Draw();
	rightArm_->Draw();

	DrawMissileVolley();
	// チャージビーム弾
	if (chargeShot_.obj) { chargeShot_.obj->Draw(); }
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

	ImGui::Separator();
	ImGui::Text("Collision Radius");
	ImGui::DragFloat("半径 (胴体)", &bodyRadius_, 0.01f, 0.0f, 100.0f);
	ImGui::DragFloat("半径 (左腕)", &leftArmRadius_, 0.01f, 0.0f, 100.0f);
	ImGui::DragFloat("半径 (右腕)", &rightArmRadius_, 0.01f, 0.0f, 100.0f);

	ImGui::DragInt("HP", &hp_);

	ImGui::Checkbox("攻撃中", &isAttack_);
	ImGui::Checkbox("怒り状態", &isEnraged_);

	ImGui::End();

#endif
}

void BossEnemy::Attack() {

	if (!player_) return;

	if (!armComboActive_) {
		return;
	}

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

				// 次フェーズへ（右→左→両手）
				if (attackPhase_ == AttackPhase::SingleRight) {
					attackPhase_ = AttackPhase::SingleLeft;  // 次は左
				} else {
					attackPhase_ = AttackPhase::BothHands;   // 次は両手
				}
			} else {
				Vector3 dirToOrigin = MyMath::Normalize(toOrigin);
				float step = std::min(armReturnSpeedSingle_, dist);
				armPos += dirToOrigin * step;
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
		const float returnSpeed = armReturnSpeedBoth_;
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
				Vector3 dirToBase = MyMath::Normalize(toBase);
				float step = std::min(returnSpeed, dist);
				leftWorldPos += dirToBase * step;
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
				Vector3 dirToBase = MyMath::Normalize(toBase);
				float step = std::min(returnSpeed, dist);
				rightWorldPos += dirToBase * step;
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
			pendingChargeAfterRetreat_ = true;
			pendingMeteorAfterCharge_ = true;

			// 腕コンボ完了
			armComboActive_ = false;
			armComboFinished_ = true;

			// Managerが次を決めるので、ここでは何もしない状態に戻す
			attackPhase_ = AttackPhase::None;

			StartRetreatAttack();
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

void BossEnemy::StartArmCombo() {

	// すでに実行中なら無視
	if (armComboActive_) return;

	armComboActive_ = true;
	armComboFinished_ = false;

	// コンボ開始は「右→左→両手」
	attackPhase_ = AttackPhase::SingleRight;

	// 片手用
	isExtending_ = true;

	// 両手用
	leftExtending_ = true;
	rightExtending_ = true;

	// ヒット数リセット
	leftArmHitCount_ = 0;
	rightArmHitCount_ = 0;
}

bool BossEnemy::ConsumeArmComboFinished() {

	if (!armComboFinished_) return false;
	armComboFinished_ = false;
	return true;
}

void BossEnemy::CancelAttacksForMeteor() {

	// 腕コンボ停止
	armComboActive_ = false;
	armComboFinished_ = false;
	attackPhase_ = AttackPhase::None;
	isExtending_ = true;
	leftExtending_ = true;
	rightExtending_ = true;

	// ミサイル停止
	missilePhase_ = MissilePhase::None;
	missileT_ = 0.0f;
	for (auto& m : missiles_) {
		m.obj.reset();
		m.bullet.reset();
		m.launched = false;
	}
	missileStartedThisRetreat_ = false;

	// チャージ停止（マーカーも含む）
	chargeActive_ = false;
	chargeShotLife_ = 0.0f;
	chargeShot_.obj.reset();
	chargeShot_.bullet.reset();
}

bool BossEnemy::ConsumeMeteorRequest() {

	if (meteorRequest_) {
		meteorRequest_ = false;
		return true;
	}
	return false;
}

void BossEnemy::OnMeteorFinished() {

	// 次は左片手攻撃から再開（← この状態を「退避の後にやる」）
	attackPhase_ = AttackPhase::SingleLeft;
	isExtending_ = true;

	// 片手フェーズ用にヒット数リセット
	leftArmHitCount_ = 0;
	rightArmHitCount_ = 0;

	// 両手フェーズ用フラグも初期化
	leftExtending_ = true;
	rightExtending_ = true;

	// 腕位置を基準に戻しておく
	leftArmPos_ = { -4.0f, 0.0f, 0.0f };
	rightArmPos_ = { 4.0f, 0.0f, 0.0f };
	if (leftArm_)  leftArm_->SetTranslate(leftArmPos_);
	if (rightArm_) rightArm_->SetTranslate(rightArmPos_);

	pendingChargeAfterRetreat_ = false;
	pendingMeteorAfterCharge_ = false;
}

void BossEnemy::HPDraw() {

	// HPバーを描画
	hpSprite_->Draw();

	// HPチップを描画
	for (auto& chip : hpChips_) {
		if (chip.sprite) {
			chip.sprite->Draw();
		}
	}
}

void BossEnemy::SetEnraged(bool enraged) {

	// 同じ状態なら何もしない
	if (isEnraged_ == enraged) {
		return;
	}

	isEnraged_ = enraged;

	if (isEnraged_) {
		attackSpeed_ = baseAttackSpeed_ * enragedArmSpeedMul_;
		armReturnSpeedSingle_ = baseArmReturnSpeedSingle_ * enragedArmSpeedMul_;
		armReturnSpeedBoth_ = baseArmReturnSpeedBoth_ * enragedArmSpeedMul_;
	}
}

Vector3 BossEnemy::GetTranslate() const { return transform_.translate; }

void BossEnemy::SetRotate(Vector3& rotate) {
	transform_.rotate = rotate;
	object3d_->SetRotate(rotate);
	leftArm_->SetRotate(rotate);
	rightArm_->SetRotate(rotate);
}

// ---- 右手 ---- //
void BossEnemy::SetRightHandScale(const Vector3& s) {

	if (rightArm_) {
		rightArm_->SetScale(s);
		// 
		rightArm_->SetRadius(1.0f * rightArm_->GetScale().x);
	}
}

Vector3 BossEnemy::GetRightHandWorldPos() const {

	if (rightArm_) {
		// 子オブジェクトなので 正確なワールド位置を取る
		return rightArm_->GetWorldPosition();
	}
	// フォールバック（親＋ローカル）
	return transform_.translate + rightArmPos_;
}

float BossEnemy::GetRightHandRadius() const {

	if (rightArm_) {
		// 子オブジェクトなので Radius を取るのが正確
		return rightArm_->GetRadius();
	}
	// フォールバック
	return 1.0f;
}

// ---- 左手 ---- //
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

	// 被弾シェイク開始
	StartBodyHitShake();

	// 胴体フラッシュ開始
	StartBodyHitFlash();

	// ダメージ前の幅
	float prevRatio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
	prevRatio = std::clamp(prevRatio, 0.0f, 1.0f);
	float prevWidth = 700.0f * prevRatio;

	// HPを減らす
	hp_ = std::max(0, hp_ - v);

	// ダメージ後の幅
	float newRatio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
	newRatio = std::clamp(newRatio, 0.0f, 1.0f);
	float newWidth = 700.0f * newRatio;

	// 減ったぶんからチップ生成
	if (hpSprite_ && prevWidth > newWidth) {
		SpawnHpChips(prevWidth, newWidth);
	}
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

Vector3 BossEnemy::GetCollisionPosition() const {

	// 
	if (object3d_) return object3d_->GetWorldPosition();
	return transform_.translate;
}

float BossEnemy::GetCollisionRadius() const {

	// 
	return bodyRadius_;   // 胴体の大きさ
}

CollisionLayer BossEnemy::GetCollisionLayer() const {

	// 
	return CollisionLayer::Enemy;
}

void BossEnemy::PartCollider::OnCollision(ICollisionObject* other) {

    if (owner->invulnerable_) {
        return; // 退避中は無敵
    }

    // プレイヤー弾以外は無視（事故防止）
    if (other->GetCollisionLayer() != CollisionLayer::PlayerBullet) {
        return;
    }

    // 弾のダメージ取得
    int dmg = 1;
    if (auto* pb = dynamic_cast<PlayerBullet*>(other)) {
        dmg = std::max(1, pb->GetDamage());
    }

    switch (part) {

    case Part::Body:
        owner->Damage(dmg);                 // ←固定1→弾のダメージ
        owner->StartBodyHitShake();
        owner->StartBodyHitFlash();
        break;

    case Part::LeftArm:
        // 腕は「ヒット数」で壊れる仕様なので、dmg分ヒットを加算
        for (int i = 0; i < dmg; ++i) {
            owner->AddHitLeftArm();
        }
        owner->StartLeftArmHitShake();
        owner->StartLeftHitFlash();
        break;

    case Part::RightArm:
        for (int i = 0; i < dmg; ++i) {
            owner->AddHitRightArm();
        }
        owner->StartRightArmHitShake();
        owner->StartRightHitFlash();
        break;
    }
}

void BossEnemy::OnCollision(ICollisionObject* other) {

	other;
}

Vector3 BossEnemy::PartCollider::GetCollisionPosition() const {

	switch (part) {

	case Part::Body:
		return owner->object3d_->GetWorldPosition();
	case Part::LeftArm:
		return owner->leftArm_->GetWorldPosition();
	case Part::RightArm:
		return owner->rightArm_->GetWorldPosition();
	}
	return {};
}

float BossEnemy::PartCollider::GetCollisionRadius() const
{
	switch (part) {

	case Part::Body:
		return owner->bodyRadius_;
	case Part::LeftArm:
		return owner->leftArmRadius_ * owner->leftArm_->GetScale().x;
	case Part::RightArm:
		return owner->rightArmRadius_ * owner->rightArm_->GetScale().x;
	}
	return 1.0f;
}

void BossEnemy::SpawnHpChips(float prevWidth, float newWidth) {

	if (!hpSprite_) return;

	float lost = prevWidth - newWidth;
	if (lost <= 0.0f) return;

	// 減った幅に応じて個数を決める（25pxで1個くらい）
	int count = static_cast<int>(lost / 25.0f) + 1;
	count = std::min(count, 30); // 上限 30 個くらい

	// HPバーの左端（アンカーは左中央）
	Vector2 basePos = hpSprite_->GetPosition();

	// 出現X範囲：減ったところ (newWidth ~ prevWidth)
	float xMin = basePos.x + newWidth;
	float xMax = basePos.x + prevWidth;

	for (int i = 0; i < count; ++i) {

		HpChip chip{};

		chip.sprite = std::make_unique<Sprite>();
		chip.sprite->Init("./Resources/images/hp.png", BlendType::BLEND_ALPHA);
		chip.sprite->SetAnchorPoint({ 0.5f, 0.5f });

		// 小さめの四角
		float w = MyMath::Rand(6.0f, 12.0f);
		float h = MyMath::Rand(6.0f, 12.0f);
		chip.sprite->SetSize({ w, h });

		// 緑色に着色（少し明るめ）
		chip.sprite->SetColor({ 0.2f, 1.0f, 0.2f, 1.0f });

		// 生成位置：減った部分のどこか＋少し上下にランダム
		float x = MyMath::Rand(xMin, xMax);
		float y = basePos.y + MyMath::Rand(-4.0f, 4.0f);
		chip.pos = { x, y };

		// 最初の速度：ちょっと横に散って、少し上に飛んでから落ちる
		chip.vel.x = MyMath::Rand(-120.0f, 120.0f);   // 横
		chip.vel.y = MyMath::Rand(-260.0f, -160.0f);  // 上方向(マイナス)

		// 寿命（秒）
		chip.life = MyMath::Rand(0.5f, 0.9f);

		chip.sprite->SetPosition(chip.pos);
		chip.sprite->Update();

		hpChips_.push_back(std::move(chip));
	}
}

void BossEnemy::DamageShake() {

	// === 胴体の元の位置（シェイク前） ===
	Vector3 baseBodyPos = transform_.translate;

	// === シェイク後の胴体位置を計算 ===
	Vector3 bodyPos = baseBodyPos;

	// 落下シェイク（撃破演出）
	if (hp_ <= 0 && !hasLanded_) {
		float sx = std::sin(fallShakeTime_ * 40.0f) * fallShakeAmplitude_;
		float sz = std::cos(fallShakeTime_ * 55.0f) * fallShakeAmplitude_;
		bodyPos.x += sx;
		bodyPos.z += sz;
	}

	// 胴体 被弾シェイク
	if (bodyHitShakeTime_ > 0.0f) {
		float t = bodyHitShakeTime_ / hitShakeDuration_;
		float amp = hitShakeAmplitude_ * t;

		bodyPos.x += MyMath::Rand(-amp, amp);
		bodyPos.y += MyMath::Rand(-amp, amp);
		bodyPos.z += MyMath::Rand(-amp, amp);
	}

	// 胴体の最終位置反映
	object3d_->SetTranslate(bodyPos);
	object3d_->SetRotate(transform_.rotate);

	// === 胴体シェイク分の offset ===
	Vector3 offset = bodyPos - baseBodyPos;

	// =================================================================
	// 左腕のシェイク処理を追加
	// =================================================================
	Vector3 leftPos = leftArmPos_;

	if (leftHitShakeTime_ > 0.0f) {
		float t = leftHitShakeTime_ / hitShakeDuration_;
		float amp = hitShakeAmplitude_ * t;

		leftPos.x += MyMath::Rand(-amp, amp);
		leftPos.y += MyMath::Rand(-amp, amp);
		leftPos.z += MyMath::Rand(-amp, amp);
	}

	leftPos = leftPos - offset;

	leftArm_->SetTranslate(leftPos);
	leftArm_->SetRotate(leftArmRot_);

	// =================================================================
	// 右腕のシェイク処理を追加
	// =================================================================
	Vector3 rightPos = rightArmPos_;

	if (rightHitShakeTime_ > 0.0f) {
		float t = rightHitShakeTime_ / hitShakeDuration_;
		float amp = hitShakeAmplitude_ * t;

		rightPos.x += MyMath::Rand(-amp, amp);
		rightPos.y += MyMath::Rand(-amp, amp);
		rightPos.z += MyMath::Rand(-amp, amp);
	}

	rightPos = rightPos - offset;

	rightArm_->SetTranslate(rightPos);
	rightArm_->SetRotate(rightArmRot_);

	// ==============================
	// 部位ごとのフラッシュ色を反映
	// ==============================
	Vector4 baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector4 flashColor = { 1.0f, 0.2f, 0.2f, 1.0f };

	// 胴体
	Vector4 bodyColor = baseColor;
	if (bodyHitFlashTime_ > 0.0f) {
		bodyColor = flashColor;
	}
	if (object3d_) {
		object3d_->SetColor(bodyColor);
	}

	// 左腕
	Vector4 leftColor = baseColor;
	if (leftHitFlashTime_ > 0.0f) {
		leftColor = flashColor;
	}
	if (leftArm_) {
		leftArm_->SetColor(leftColor);
	}

	// 右腕
	Vector4 rightColor = baseColor;
	if (rightHitFlashTime_ > 0.0f) {
		rightColor = flashColor;
	}
	if (rightArm_) {
		rightArm_->SetColor(rightColor);
	}
}

void BossEnemy::StartRetreatAttack() {

	if (retreatPhase_ != RetreatPhase::None) return;
	if (hp_ <= 0) return;

	retreatPhase_ = RetreatPhase::MoveOut;
	retreatT_ = 0.0f;

	retreatStartPos_ = transform_.translate;

	retreatBackPos_ = retreatStartPos_;
	retreatBackPos_.z += retreatBackZOffset_;
	retreatBackPos_.y += retreatUpOffset_;

	invulnerable_ = true;

	// Stay内部状態を毎回リセット
	retreatStayPhase_ = RetreatStayPhase::Unflatten;

	// 退避開始時に腕を基準位置に戻す
	leftArmPos_ = { -4.0f, 0.0f, 0.0f };
	rightArmPos_ = { 4.0f, 0.0f, 0.0f };
	if (leftArm_)  leftArm_->SetTranslate(leftArmPos_);
	if (rightArm_) rightArm_->SetTranslate(rightArmPos_);

	missileStartedThisRetreat_ = false;
}

void BossEnemy::StopAllAttacksOnDeath() {

	combatEnabled_ = false;
	isAttack_ = false;
	attackPhase_ = AttackPhase::None;

	// チャージ状態を止める
	chargeActive_ = false;
	chargeRequest_ = false;
	pendingChargeAfterRetreat_ = false;
	pendingMeteorAfterCharge_ = false;

	// 腕クロスを即戻し
	if (chargePoseSaved_) {
		leftArmPos_ = chargeSavedLeftArmPos_;
		rightArmPos_ = chargeSavedRightArmPos_;
		if (leftArm_)  leftArm_->SetTranslate(leftArmPos_);
		if (rightArm_) rightArm_->SetTranslate(rightArmPos_);
		chargePoseSaved_ = false;
		chargePoseLerp_ = 0.0f;
	}

	// ミサイル全消し（hp<=0 だと UpdateMissileVolley が呼ばれず消えないため）
	missilePhase_ = MissilePhase::None;
	missileT_ = 0.0f;
	missileHitPlayer_ = false;
	for (auto& m : missiles_) {
		if (collisionManager_ && m.bullet) {
			collisionManager_->Unregister(m.bullet.get());
		}
		m.bullet.reset();
		m.obj.reset();
		m.launched = false;
	}

	// チャージ弾（見た目＋当たり判定）全消し
	if (collisionManager_ && chargeShot_.bullet) {
		collisionManager_->Unregister(chargeShot_.bullet.get());
	}
	chargeShot_.bullet.reset();
	chargeShot_.obj.reset();
	chargeShotLife_ = 0.0f;
	chargeShotHitOnce_ = false;
}

void BossEnemy::UpdateRetreat(float dt) {

	if (retreatPhase_ == RetreatPhase::None) return;

	retreatT_ += dt;

	auto ApplyScaleFactorXZ_Y = [this](float factorXZ, float factorY) {

		// 0.0f を許可したいけど、内部計算の安全のために極小値へクランプ
		const float kEps = 0.001f;
		factorXZ = std::max(factorXZ, kEps);

		Vector3 bodyS = {
			baseBodyScale_.x * factorXZ,
			baseBodyScale_.y * factorY,
			baseBodyScale_.z * factorXZ
		};

		Vector3 armS = {
			baseArmScale_.x * factorXZ,
			baseArmScale_.y * factorY,
			baseArmScale_.z * factorXZ
		};

		if (object3d_) object3d_->SetScale(bodyS);
		if (leftArm_)  leftArm_->SetScale(armS);
		if (rightArm_) rightArm_->SetScale(armS);
		};

	switch (retreatPhase_) {

	case RetreatPhase::MoveOut:
	{

		// まず縮むだけ（retreatShrinkTime_）
		if (retreatT_ < retreatShrinkTime_) {

			float u = (retreatShrinkTime_ <= 0.0f) ? 1.0f : (retreatT_ / retreatShrinkTime_);
			u = MyMath::Clamp01(u);

			// 横だけ強めに潰す（ワープ感）
			float eXZ = MyMath::EaseInOutCubic(u);
			float eY = MyMath::EaseOutCubic(u); // Yは軽く（変化を弱めたいなら EaseOut が無難）

			float factorXZ = MyMath::Lerp(1.0f, retreatMinScaleXZ_, eXZ);
			float factorY = MyMath::Lerp(1.0f, retreatMinScaleY_, eY);

			ApplyScaleFactorXZ_Y(factorXZ, factorY);

			// 位置は動かさない（ここ重要）
			transform_.translate = retreatStartPos_;
			break;
		}

		// 縮み終わったら移動だけ（retreatMoveTime_）
		float moveT = retreatT_ - retreatShrinkTime_;

		float u = (retreatMoveTime_ <= 0.0f) ? 1.0f : (moveT / retreatMoveTime_);
		u = MyMath::Clamp01(u);

		float e = MyMath::EaseInOutCubic(u);

		// スケールは最小固定のまま
		ApplyScaleFactorXZ_Y(retreatMinScaleXZ_, retreatMinScaleY_);

		// ここで初めて移動
		transform_.translate = MyMath::Lerp(retreatStartPos_, retreatBackPos_, e);

		if (u >= 1.0f) {
			retreatPhase_ = RetreatPhase::Stay;
			retreatT_ = 0.0f;

			if (pendingChargeAfterRetreat_) {
				pendingChargeAfterRetreat_ = false;

				RequestChargeAttack(nextChargeTargetLeft_);
				nextChargeTargetLeft_ = !nextChargeTargetLeft_;
			}
		}

	} break;


	case RetreatPhase::Stay:
	{

		// 奥位置固定
		transform_.translate = retreatBackPos_;

		if (retreatStayPhase_ == RetreatStayPhase::Unflatten) {

			// 奥で「ペラペラ → 通常」に戻す
			float u = (retreatUnflattenTime_ <= 0.0f) ? 1.0f : (retreatT_ / retreatUnflattenTime_);
			u = MyMath::Clamp01(u);

			// 出現感：最初ゆっくり→途中早い→最後ゆっくり
			float e = MyMath::EaseInOutCubic(u);

			// XZは0→1へ（Yはほぼ固定 or ちょいだけ戻す）
			float factorXZ = MyMath::Lerp(retreatMinScaleXZ_, 1.0f, e);
			float factorY = MyMath::Lerp(retreatMinScaleY_, 1.0f, e * 0.5f); // Yは変化少なめ

			ApplyScaleFactorXZ_Y(factorXZ, factorY);

			if (u >= 1.0f) {
				retreatStayPhase_ = RetreatStayPhase::Hold;
				retreatT_ = 0.0f;

				// 念のため完全通常
				ApplyScaleFactorXZ_Y(1.0f, 1.0f);

				// 奥に到達＆通常に戻った“直後”にミサイル開始（この退避中に1回だけ）
				if (!missileStartedThisRetreat_) {
					StartMissileVolley();
					missileStartedThisRetreat_ = true;
				}
			}

		} else { // Hold

			// 奥で攻撃中（Hold）
			ApplyScaleFactorXZ_Y(1.0f, 1.0f);
			transform_.translate = retreatBackPos_;

			// ミサイルが終わったら戻る（全滅 or 命中）
			if (missileStartedThisRetreat_ && missilePhase_ == MissilePhase::None) {
				retreatPhase_ = RetreatPhase::Return;
				retreatT_ = 0.0f;
			}
		}
	} break;

	case RetreatPhase::Return:
	{

		// ① まず奥で「普通 → ペラ」へ（ここが無いとパッと0になる）
		if (retreatT_ < retreatFlattenTime_) {

			float u = (retreatFlattenTime_ <= 0.0f) ? 1.0f : (retreatT_ / retreatFlattenTime_);
			u = MyMath::Clamp01(u);

			float eXZ = MyMath::EaseInOutCubic(u);
			float eY = MyMath::EaseOutCubic(u);

			float factorXZ = MyMath::Lerp(1.0f, retreatMinScaleXZ_, eXZ);
			float factorY = MyMath::Lerp(1.0f, retreatMinScaleY_, eY);

			ApplyScaleFactorXZ_Y(factorXZ, factorY);

			// 位置は奥に固定
			transform_.translate = retreatBackPos_;
			break;
		}

		// ② ペラのまま移動して戻る
		float moveT = retreatT_ - retreatFlattenTime_;
		if (moveT < retreatMoveTime_) {

			float u = (retreatMoveTime_ <= 0.0f) ? 1.0f : (moveT / retreatMoveTime_);
			u = MyMath::Clamp01(u);

			float e = MyMath::EaseInOutCubic(u);

			ApplyScaleFactorXZ_Y(retreatMinScaleXZ_, retreatMinScaleY_);
			transform_.translate = MyMath::Lerp(retreatBackPos_, retreatStartPos_, e);
			break;
		}

		// ③ 手前に戻ったら「ペラ → 普通」へ
		float growT = moveT - retreatMoveTime_;

		float u = (retreatGrowTime_ <= 0.0f) ? 1.0f : (growT / retreatGrowTime_);
		u = MyMath::Clamp01(u);

		float eXZ = MyMath::EaseInOutCubic(u);
		float eY = MyMath::EaseOutCubic(u);

		float factorXZ = MyMath::Lerp(retreatMinScaleXZ_, 1.0f, eXZ);
		float factorY = MyMath::Lerp(retreatMinScaleY_, 1.0f, eY * 0.5f); // Yは変化少なめ

		ApplyScaleFactorXZ_Y(factorXZ, factorY);
		transform_.translate = retreatStartPos_;

		if (u >= 1.0f) {
			retreatPhase_ = RetreatPhase::None;
			retreatT_ = 0.0f;
			invulnerable_ = false;

			ApplyScaleFactorXZ_Y(1.0f, 1.0f);
			transform_.translate = retreatStartPos_;
		}

	} break;

	default:
		break;
	}
}

void BossEnemy::StartMissileVolley() {

	missileHitPlayer_ = false;

	if (missilePhase_ != MissilePhase::None) return;
	if (!camera_ || !player_) return;

	missilePhase_ = MissilePhase::Telegraph;
	missileT_ = 0.0f;

	for (auto& m : missiles_) {
		m.launched = false;

		m.obj = std::make_unique<Object3d>();
		m.obj->Init(BlendType::BLEND_NONE);
		m.obj->SetModel("sphere.obj");
		m.obj->SetDefaultCamera(camera_);
		m.obj->SetScale({ 1.0f, 1.0f, 1.0f });

		m.bullet = std::make_unique<EnemyBullet>();
		m.bullet->Init(camera_, m.obj.get());
		m.bullet->SetTranlate(transform_.translate);
		m.bullet->SetDirection({ 0.0f, 0.0f, 0.0f }); // 予告中は動かない
		m.bullet->SetSpeed(0.0f);

		if (collisionManager_) {
			collisionManager_->Register(m.bullet.get());
		}
	}
}

void BossEnemy::UpdateMissileVolley(float dt) {

	if (missilePhase_ == MissilePhase::None) return;

	missileT_ += dt;

	const Vector3 bossPos = transform_.translate;

	// 予告：上半円に配置して“表示だけ”
	if (missilePhase_ == MissilePhase::Telegraph) {

		for (int i = 0; i < 4; ++i) {
			float t = (float)i / 3.0f;       // 0, 1/3, 2/3, 1
			float rad = t * 3.14159265f;     // 0..π

			// 半円を X-Y 平面に（必要なら X-Z にしてもOK）
			Vector3 offset{};
			offset.x = std::cos(rad) * missileRadius_;
			offset.y = std::sin(rad) * missileRadius_ + missileHeight_;
			offset.z = 0.0f;

			Vector3 p = bossPos + offset;

			missiles_[i].bullet->SetTranlate(p);
			missiles_[i].bullet->SetDirection({ 0,0,0 });
			missiles_[i].bullet->SetSpeed(0.0f);

			missiles_[i].bullet->Update();
		}

		if (missileT_ >= missileTelegraphTime_) {
			// 発射へ
			missilePhase_ = MissilePhase::Launch;
			missileT_ = 0.0f;

			// 発射方向をセット
			const Vector3 playerPos = player_->GetTransform().translate;

			for (auto& m : missiles_) {
				Vector3 from = m.bullet->GetTranslate();
				Vector3 dir = playerPos - from;
				dir = MyMath::Normalize(dir);

				m.bullet->SetDirection(dir);
				m.bullet->SetSpeed(missileSpeed_);
				m.launched = true;
			}
		}
		return;
	}

	// 発射：EnemyBullet の Update() に任せる（追尾にしたいならここでdir更新）
	if (missilePhase_ == MissilePhase::Launch) {

		if (missileT_ >= missileLaunchTimeout_) {
			missilePhase_ = MissilePhase::None;

			for (auto& m : missiles_) {
				if (collisionManager_ && m.bullet) {
					collisionManager_->Unregister(m.bullet.get());
				}
				m.bullet.reset();
				m.obj.reset();
				m.launched = false;
			}
			return;
		}

		int aliveCount = 0;
		const Vector3 playerPos = player_->GetTransform().translate;

		for (auto& m : missiles_) {
			if (!m.bullet) { continue; }

			m.bullet->Update();

			if (m.bullet->IsDead()) {

				// Playerに当たったか
				if (m.bullet->DidHitPlayer()) {
					missileHitPlayer_ = true;
				}

				if (collisionManager_ && m.bullet) {
					collisionManager_->Unregister(m.bullet.get());
				}
				m.bullet.reset();
				m.obj.reset();
				m.launched = false;
				continue;
			}

			++aliveCount;

			const Vector3 p = m.bullet->GetTranslate();

			// --- 命中判定（簡易：距離） --- //
			Vector3 d{ playerPos.x - p.x, playerPos.y - p.y, playerPos.z - p.z };
			const float dist2 = d.x * d.x + d.y * d.y + d.z * d.z;

			if (dist2 <= missileHitDist_ * missileHitDist_) {
				missileHitPlayer_ = true;

				// 命中したらこの弾は消す（演出上）
				if (collisionManager_ && m.bullet) {
					collisionManager_->Unregister(m.bullet.get());
				}
				m.bullet.reset();
				m.obj.reset();
				--aliveCount; // 消したのでaliveを調整
				break;        // 1発当たったら即終了でOKなら break
			}

			// --- 遠すぎたら消す（全滅条件に寄与）---
			const float max2 = missileMaxDist_ * missileMaxDist_;
			if (dist2 >= max2) {
				if (collisionManager_ && m.bullet) {
					collisionManager_->Unregister(m.bullet.get());
				}
				m.bullet.reset();
				m.obj.reset();
				--aliveCount;
			}
		}

		// 終了条件：命中 or 全滅
		if (missileHitPlayer_ || aliveCount <= 0) {
			missilePhase_ = MissilePhase::None;

			// 念のため全部解放
			for (auto& m : missiles_) {
				if (collisionManager_ && m.bullet) {
					collisionManager_->Unregister(m.bullet.get());
				}
				m.bullet.reset();
				m.obj.reset();
				m.launched = false;
			}
		}
	}
}

void BossEnemy::DrawMissileVolley() {

	if (missilePhase_ == MissilePhase::None) return;

	for (auto& m : missiles_) {
		if (m.bullet) {
			m.bullet->Draw();
		}
	}
}

void BossEnemy::RequestChargeAttack(bool targetLeft) {

	chargeRequest_ = true;
	chargeTargetLeft_ = targetLeft;
}

bool BossEnemy::ConsumeChargeRequest() {

	if (chargeRequest_) {
		chargeRequest_ = false;
		return true;
	}
	return false;

}

void BossEnemy::OnChargeAttackFinished() {

	chargeActive_ = false;

	if (pendingMeteorAfterCharge_) {
		pendingMeteorAfterCharge_ = false;
		meteorRequest_ = true;
		attackPhase_ = AttackPhase::WaitMeteor;
	}
}

bool BossEnemy::IsChargeBeamShotActive() const {

	// 発射中か
	return chargeShot_.bullet != nullptr;
}

void BossEnemy::StartChargeBeamShot(bool useLeftArm) {
	

	// 撃破後は生成しない
	if (hp_ <= 0) { return; }
	if (!combatEnabled_) { return; }

	// 既に発射中なら上書きしない
	if (chargeShot_.bullet) { return; }
	if (!camera_ || !player_) { return; }
	// 生成位置：対象腕の先端付近
	Vector3 spawnPos = useLeftArm ? GetLeftHandWorldPos() : GetRightHandWorldPos();
	Vector3 playerPos = player_->GetTransform().translate;

	// 見た目（球）
	chargeShot_.obj = std::make_unique<Object3d>();
	chargeShot_.obj->Init(BlendType::BLEND_NONE);
	chargeShot_.obj->SetModel("sphere.obj");
	chargeShot_.obj->SetDefaultCamera(camera_);
	chargeShot_.obj->SetTranslate(spawnPos);

	chargeShot_.bullet = std::make_unique<EnemyBullet>();
	chargeShot_.bullet->Init(camera_, chargeShot_.obj.get());
	chargeShot_.bullet->SetDestroyOnPlayerHit(false);
	chargeShot_.bullet->SetTranlate(spawnPos);

	chargeShot_.obj->SetScale(chargeBeamStartScale_);

	Vector3 dir = MyMath::Normalize(playerPos - spawnPos);
	chargeShot_.bullet->SetDirection(dir);
	chargeShot_.bullet->SetSpeed(1.0f); // 速め

	if (collisionManager_) {
		collisionManager_->Register(chargeShot_.bullet.get());

	}
	chargeShotLife_ = 0.0f;

	chargeShotHitOnce_ = false;
}

void BossEnemy::UpdateChargeBeamShot(float dt) {

	if (!chargeShot_.bullet) { return; }

	chargeShotLife_ += dt;

	if (chargeShot_.obj) {

		float t = chargeShotLife_ / chargeShotMaxLife_;
		t = std::clamp(t, 0.0f, 1.0f);

		// 1→0に減る係数（ゆっくり縮む感じ）
		float k = 1.0f - t;
		float ease = k * k;

		Vector3 start = chargeBeamStartScale_;
		Vector3 end = chargeBeamEndScale_;

		Vector3 s;
		s.x = end.x + (start.x - end.x) * ease;
		s.y = end.y + (start.y - end.y) * ease;
		s.z = end.z + (start.z - end.z) * ease;

		chargeShot_.obj->SetScale(s);
	}

	chargeShot_.bullet->Update();

	// 
	if (!chargeShotHitOnce_ && chargeShot_.bullet->DidHitPlayer()) {
		chargeShotHitOnce_ = true;

		// 以後は当たり判定しない（ビームは残す）
		if (collisionManager_) {
			collisionManager_->Unregister(chargeShot_.bullet.get());
		}
	}

	bool end = false;

	// 発射から一定秒数は絶対に消さない
	if (chargeShotLife_ >= chargeShotMinLife_) {

		// ヒットで消すのはやめる(今後変更の可能性あり)
		// if (chargeShot_.bullet->DidHitPlayer()) { end = true; }

		// 
		/*if (chargeShot_.bullet->IsDead()) {
			end = true;
		}*/

		// 最大寿命で終了
		if (chargeShotLife_ >= chargeShotMaxLife_) {
			end = true;
		}
	}

	if (end) {
		if (collisionManager_ && chargeShot_.bullet) {
			collisionManager_->Unregister(chargeShot_.bullet.get());
		}
		chargeShot_.bullet.reset();
		chargeShot_.obj.reset();
	}
}

void BossEnemy::UpdateChargeCrossPose(float dt) {

	// チャージしていないなら、保存していた姿勢を戻して終了
	if (!chargeActive_) {
		if (chargePoseSaved_) {
			// チャージ開始時の姿勢に戻す
			leftArmPos_ = chargeSavedLeftArmPos_;
			rightArmPos_ = chargeSavedRightArmPos_;
			if (leftArm_) { leftArm_->SetTranslate(leftArmPos_); }
			if (rightArm_) { rightArm_->SetTranslate(rightArmPos_); }
			chargePoseSaved_ = false;
			chargePoseLerp_ = 0.0f;
		}
		return;
	}

	// 初回だけ現在姿勢を保存
	if (!chargePoseSaved_) {
		chargePoseSaved_ = true;
		chargeSavedLeftArmPos_ = leftArmPos_;
		chargeSavedRightArmPos_ = rightArmPos_;
		chargePoseLerp_ = 0.0f;
	}

	// 胴体の前でクロス（左右のXを入れ替える）
	const Vector3 leftTarget{ -0.5f, 0.5f, -chargeCrossZOffset_ };
	const Vector3 rightTarget{ +0.5f, 0.5f, -(chargeCrossZOffset_) };

	chargePoseLerp_ += dt * chargePoseInSpeed_;
	if (chargePoseLerp_ > 1.0f) { chargePoseLerp_ = 1.0f; }

	auto lerp3 = [](const Vector3& a, const Vector3& b, float t) {
		return Vector3{
			a.x + (b.x - a.x) * t,
			a.y + (b.y - a.y) * t,
			a.z + (b.z - a.z) * t,
		};
		};

	leftArmPos_ = lerp3(chargeSavedLeftArmPos_, leftTarget, chargePoseLerp_);
	rightArmPos_ = lerp3(chargeSavedRightArmPos_, rightTarget, chargePoseLerp_);

	if (leftArm_) { leftArm_->SetTranslate(leftArmPos_); }
	if (rightArm_) { rightArm_->SetTranslate(rightArmPos_); }
}

void BossEnemy::ChargeEffect(float dt) {

	if (chargeActive_) {

		// ★ 手の位置（ワールド）から、中央（両手の中間）を作る
		Vector3 leftW = GetLeftHandWorldPos();
		Vector3 rightW = GetRightHandWorldPos();

		Vector3 fxPos = (leftW + rightW) * 0.5f;

		// 少し手前/上に寄せたいならここで調整
		// fxPos.y += 0.3f;

		auto* pm = ParticleManager::GetInstance();

		const bool hasCore = pm->Exists("charge_core");
		const bool hasPulse = pm->Exists("charge_pulse");

		chargeFxCoreTimer_ += dt;
		if (chargeFxCoreTimer_ >= 0.035f) {
			chargeFxCoreTimer_ = 0.0f;
			if (hasCore) pm->Emit("charge_core", fxPos, 6);
		}

		chargeFxPulseTimer_ += dt;
		if (chargeFxPulseTimer_ >= 0.18f) {
			chargeFxPulseTimer_ = 0.0f;
			if (hasPulse) pm->Emit("charge_pulse", fxPos, 1);
		}

	} else {
		chargeFxCoreTimer_ = 0.0f;
		chargeFxPulseTimer_ = 0.0f;
	}
}