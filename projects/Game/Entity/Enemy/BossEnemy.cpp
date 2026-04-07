#include "BossEnemy.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif // USE_IMGUI

#include "Game/Entity/Player/Player.h"
#include "Game/Entity/Player/PlayerBullet.h"
#include "Engine/Base/System/System.h"
#include "Engine/Base/Particle/ParticleManager.h"

#include <cmath>

// モデル
static const char* kBossCoreModel = "BossEnemyCore.obj";
static const char* kBossArmorModel = "BossArmor.obj";


void BossEnemy::SetTranslate(Vector3 translate) { transform_.translate = translate; }

void BossEnemy::Init(Camera* camera) {

	// カメラの設定
	camera_ = camera;

	// 自機オブジェクトの生成
	object3d_ = std::make_unique<Object3d>();
	object3d_->Init(BlendType::BLEND_NONE);

	object3d_->SetModel(kBossCoreModel);
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
	// 腕は腕攻撃中だけ表示したいので初期は非表示（スケール0 + 半径0にする）
	leftArmVisible_ = false;
	rightArmVisible_ = false;
	leftArm_->SetScale({ 0.0f, 0.0f, 0.0f });
	rightArm_->SetScale({ 0.0f, 0.0f, 0.0f });


	// 
	hpSprite_ = std::make_unique<Sprite>();
	hpSprite_->Init("./Resources/images/hp.png", BlendType::BLEND_NONE);
	hpSprite_->SetAnchorPoint({ 0.0f, 0.5f });
	hpSprite_->SetSize({ 700.0f,50.0f });
	hpSprite_->SetPosition({ 200.0f,100.0f });

	// チャージコア、チャージビームの生成
	chargeCore_ = std::make_unique<BossChargeCore>();
	chargeCore_->Init(camera_);

	chargeBeam_ = std::make_unique<BossChargeBeam>();
	chargeBeam_->Init(camera_);

	// 当たり判定コライダーの設定
	bodyCol_.owner = this;
	bodyCol_.part = PartCollider::Part::Body;

	leftCol_.owner = this;
	leftCol_.part = PartCollider::Part::LeftArm;

	rightCol_.owner = this;
	rightCol_.part = PartCollider::Part::RightArm;


	// 装甲（周回）を生成
	InitArmors();

	// 腕は攻撃時のみ表示
	leftArmVisible_ = false;
	rightArmVisible_ = false;
	leftArm_->SetScale({ 0.0f,0.0f,0.0f });
	rightArm_->SetScale({ 0.0f,0.0f,0.0f });

	// --- 怒り用：通常時の基準値を保存 ---
	baseAttackSpeed_ = attackSpeed_;
	baseArmReturnSpeedSingle_ = armReturnSpeedSingle_;
	baseArmReturnSpeedBoth_ = armReturnSpeedBoth_;
}

void BossEnemy::Update() {

	const float dt = KomEngine::System::GetDeltaTime();

	UpdateChargeCrossPose(dt);
	UpdateEnrageTransition(dt);

	ChargeEffect(dt);

	if (chargeCore_ && chargeCore_->IsActive()) {
		chargeCore_->SetWorldPos(transform_.translate + chargeCoreOffset_);
		chargeCore_->Update(dt);
	}

	if (chargeBeam_ && chargeBeam_->IsActive()) {
		chargeBeam_->Update(dt);
	}

	// 3Dオブジェクト更新
	object3d_->Update();
	leftArm_->Update();
	rightArm_->Update();

	// 装甲（周回）更新
	UpdateArmors(dt);

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

		if (!deathEffectStarted_) {
			StartDeathEffect();
		}

		if (!hasLanded_) {
			UpdateDeathEffect(dt);
		}
	}
	else {
		// 生きている間の従来処理

		if (KomEngine::System::GetInput()->PushKey(DIK_SPACE)) {
			pushEnter_ = true;
		}

		if (IsRetreating()) {
			// 退避中は通常攻撃をしない
		}
		else {
			if (combatEnabled_ && isAttack_ && !chargeActive_) {
				Attack();
			}
		}

		if (pushEnter_) {
			if (isInTitleScene_) {
				TitleSceneMove();
			}
		}
	}

	if (hp_ > 0) {
		UpdateChargeBeamShot(dt);
	}

	// ---------------------- 被弾シェイク ---------------------- //
	DamageShake();

	// ---------------------- 腕の表示制御（腕攻撃中だけ） ---------------------- //
	bool showLeft = false;
	bool showRight = false;
	if (armComboActive_) {
		switch (attackPhase_) {
		case AttackPhase::SingleLeft:  showLeft = true; break;
		case AttackPhase::SingleRight: showRight = true; break;
		case AttackPhase::BothHands:   showLeft = true; showRight = true; break;
		default: break;
		}
	}
	leftArmVisible_ = showLeft;
	rightArmVisible_ = showRight;

	// 表示する腕はスケールを戻し、非表示はスケール0（当たり判定も無効化）
	const Vector3 useBodyScale = retreatVisualOverride_ ? retreatBodyScale_ : baseBodyScale_;
	const Vector3 useArmScale = retreatVisualOverride_ ? retreatArmScale_ : baseArmScale_;

	object3d_->SetScale(useBodyScale);
	leftArm_->SetScale(leftArmVisible_ ? useArmScale : Vector3{ 0.0f, 0.0f, 0.0f });
	rightArm_->SetScale(rightArmVisible_ ? useArmScale : Vector3{ 0.0f, 0.0f, 0.0f });

	object3d_->SetRadius(bodyRadius_);
	leftArm_->SetRadius(leftArmVisible_ ? (leftArmRadius_ * leftArm_->GetScale().x) : 0.0f);
	rightArm_->SetRadius(rightArmVisible_ ? (rightArmRadius_ * rightArm_->GetScale().x) : 0.0f);
}

void BossEnemy::Draw() {

	//
	object3d_->Draw();
	DrawArmors();
	if (leftArmVisible_) { leftArm_->Draw(); }
	if (rightArmVisible_) { rightArm_->Draw(); }

	// チャージビーム弾
	if (chargeShot_.obj) { chargeShot_.obj->Draw(); }

	if (chargeCore_) {
		chargeCore_->Draw();
	}
	if (chargeBeam_) {
		chargeBeam_->Draw();
	}
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

	if (ImGui::Begin("Boss Armor")) {

		ImGui::Text("=== Armor Settings ===");

		// 個数
		ImGui::SliderInt("Armor Count", &armorInitialCount_, 1, 32);

		// 回転半径
		ImGui::SliderFloat("Orbit Radius", &armorOrbitRadius_, 0.0f, 20.0f);

		// 回転速度
		ImGui::SliderFloat("Orbit Speed", &armorOrbitSpeed_, -5.0f, 5.0f);

		// 上下揺れ幅
		ImGui::SliderFloat("Float Amp", &armorFloatAmp_, 0.0f, 5.0f);

		// 上下揺れ速度
		ImGui::SliderFloat("Float Speed", &armorFloatSpeed_, 0.0f, 10.0f);

		// スケール
		float scale[3] = { armorScale_.x, armorScale_.y, armorScale_.z };
		if (ImGui::DragFloat3("Armor Scale", scale, 0.01f, 0.01f, 5.0f)) {
			armorScale_.x = scale[0];
			armorScale_.y = scale[1];
			armorScale_.z = scale[2];
		}

		// スケールを適応
		for (const auto& a : armors_) {
			a.obj->SetScale(armorScale_);
		}

		// 作り直し
		if (ImGui::Button("Rebuild Armors")) {
			armorRebuildRequest_ = true;
		}

		ImGui::SameLine();

		if (ImGui::Button("Reset")) {
			armorInitialCount_ = 12;
			armorOrbitRadius_ = 4.8f;
			armorOrbitSpeed_ = 0.9f;
			armorFloatAmp_ = 0.18f;
			armorFloatSpeed_ = 1.6f;
			armorScale_ = { 0.7f, 0.7f, 0.7f };
			armorRebuildRequest_ = true;
		}
	}
	ImGui::End();

	// 再構築
	if (armorRebuildRequest_) {
		InitArmors();
		armorRebuildRequest_ = false;
	}
	ImGui::End();

#endif
}

void BossEnemy::Attack() {

	if (!player_) return;

	// 怒り遷移中は攻撃しない
	if (enrageTransitioning_) {
		return;
	}

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

		// =====================================================
		// 予備動作
		// 1. 元位置に表示
		// 2. 後ろに引く
		// 3. 引いた位置で少し溜める
		// =====================================================
		if (armTelegraphActive_) {

			if (armTelegraphTimer_ <= 0.0f) {
				// 必ず「元の位置」から始める
				armTelegraphStartPos_ = baseLocalOffset;
				armTelegraphTargetPos_ = baseLocalOffset - dir * armTelegraphBackAmount_;

				targetPos = armTelegraphStartPos_;
				targetArm->SetTranslate(targetPos);
			}

			armTelegraphTimer_ += KomEngine::System::GetDeltaTime();

			Vector3 telegraphPos = armTelegraphStartPos_;

			// -------------------------
			// 前半：元位置 → 後ろへ引く
			// -------------------------
			if (armTelegraphTimer_ < armTelegraphBackTime_) {
				float t = armTelegraphTimer_ / armTelegraphBackTime_;
				t = std::clamp(t, 0.0f, 1.0f);

				float ease = t * t * (3.0f - 2.0f * t);
				telegraphPos = MyMath::Lerp(armTelegraphStartPos_, armTelegraphTargetPos_, ease);
			}
			// -------------------------
			// 後半：引いた位置で少し溜める
			// -------------------------
			else {
				float s = armTelegraphTimer_ - armTelegraphBackTime_;
				telegraphPos = armTelegraphTargetPos_;

				telegraphPos.x += std::sin(s * armTelegraphShakeFreq_) * armTelegraphShakeAmount_;
				telegraphPos.y += std::cos(s * armTelegraphShakeFreq_ * 1.11f) * armTelegraphShakeAmount_;
				telegraphPos.z += std::sin(s * armTelegraphShakeFreq_ * 0.91f) * armTelegraphShakeAmount_;
			}

			armPos = telegraphPos;
			targetPos = armPos;
			targetArm->SetTranslate(targetPos);

			// 予備動作終了 → 突進開始
			if (armTelegraphTimer_ >= armTelegraphDuration_) {
				armTelegraphActive_ = false;
				armTelegraphTimer_ = 0.0f;

				isExtending_ = true;
				targetPos = armTelegraphTargetPos_;
				targetArm->SetTranslate(targetPos);
			}

			break;
		}

		// =====================================================
		// 通常の片腕攻撃（突進）
		// =====================================================
		if (isExtending_) {
			// 長めにグッと前へ出る
			armPos += dir * armRushSpeed_;

			// 一定距離 or 規定ヒット数で戻りフェーズへ
			if (MyMath::Length(armPos - baseLocalOffset) >= 22.0f ||
				hitCount >= maxHitCount_) {
				isExtending_ = false;
			}
		}
		else {
			// 基本位置へ戻す
			Vector3 toOrigin = baseLocalOffset - armPos;
			float dist = MyMath::Length(toOrigin);

			if (dist < 0.5f) {
				// 戻り完了
				armPos = baseLocalOffset;
				hitCount = 0;

				// 次の片腕/両手へ
				isExtending_ = false;
				armTelegraphActive_ = true;
				armTelegraphTimer_ = 0.0f;

				if (attackPhase_ == AttackPhase::SingleRight) {
					attackPhase_ = AttackPhase::SingleLeft;
				}
				else {
					attackPhase_ = AttackPhase::BothHands;

					armTelegraphActive_ = false;
					armTelegraphTimer_ = 0.0f;

					bothTelegraphActive_ = true;
					bothTelegraphTimer_ = 0.0f;

					leftExtending_ = false;
					rightExtending_ = false;

					leftArmPos_ = { -4.0f, 0.0f, 0.0f };
					rightArmPos_ = { 4.0f, 0.0f, 0.0f };

					if (leftArm_) { leftArm_->SetTranslate(leftArmPos_); }
					if (rightArm_) { rightArm_->SetTranslate(rightArmPos_); }
				}
			}
			else {
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
		const Vector3 leftBaseLocal{ -4.0f, 0.0f, 0.0f };
		const Vector3 rightBaseLocal{ 4.0f, 0.0f, 0.0f };

		const Vector3 leftBaseWorld = transform_.translate + leftBaseLocal;
		const Vector3 rightBaseWorld = transform_.translate + rightBaseLocal;

		Vector3 leftWorldPos = leftArm_->GetWorldPosition();
		Vector3 rightWorldPos = rightArm_->GetWorldPosition();

		const Vector3 playerPos = player_->GetTranslate();

		Vector3 dirL = MyMath::Normalize(playerPos - leftBaseWorld);
		Vector3 dirR = MyMath::Normalize(playerPos - rightBaseWorld);

		const float maxLen = 22.0f;
		const float returnSpeed = armReturnSpeedBoth_;
		const float endThreshold = 0.3f;

		// =========================================
		// 両手の予備動作
		// =========================================
		if (bothTelegraphActive_) {

			if (bothTelegraphTimer_ <= 0.0f) {
				leftBothTelegraphStartPos_ = leftBaseLocal;
				rightBothTelegraphStartPos_ = rightBaseLocal;

				leftBothTelegraphTargetPos_ = leftBaseLocal - dirL * bothTelegraphBackAmount_;
				rightBothTelegraphTargetPos_ = rightBaseLocal - dirR * bothTelegraphBackAmount_;

				leftArmPos_ = leftBothTelegraphStartPos_;
				rightArmPos_ = rightBothTelegraphStartPos_;

				leftArm_->SetTranslate(leftArmPos_);
				rightArm_->SetTranslate(rightArmPos_);
			}

			bothTelegraphTimer_ += KomEngine::System::GetDeltaTime();

			Vector3 leftLocal = leftBothTelegraphStartPos_;
			Vector3 rightLocal = rightBothTelegraphStartPos_;

			// 前半：左右同時に後ろへ引く
			if (bothTelegraphTimer_ < bothTelegraphBackTime_) {
				float t = bothTelegraphTimer_ / bothTelegraphBackTime_;
				t = std::clamp(t, 0.0f, 1.0f);

				float ease = t * t * (3.0f - 2.0f * t);

				leftLocal = MyMath::Lerp(leftBothTelegraphStartPos_, leftBothTelegraphTargetPos_, ease);
				rightLocal = MyMath::Lerp(rightBothTelegraphStartPos_, rightBothTelegraphTargetPos_, ease);
			}
			// 後半：引いた位置で左右同時に振動
			else {
				float s = bothTelegraphTimer_ - bothTelegraphBackTime_;

				leftLocal = leftBothTelegraphTargetPos_;
				rightLocal = rightBothTelegraphTargetPos_;

				float shakeX = std::sin(s * bothTelegraphShakeFreq_) * bothTelegraphShakeAmount_;
				float shakeY = std::cos(s * bothTelegraphShakeFreq_ * 1.09f) * bothTelegraphShakeAmount_;
				float shakeZ = std::sin(s * bothTelegraphShakeFreq_ * 0.93f) * bothTelegraphShakeAmount_;

				leftLocal.x += shakeX;
				leftLocal.y += shakeY;
				leftLocal.z += shakeZ;

				rightLocal.x -= shakeX;
				rightLocal.y += shakeY;
				rightLocal.z += shakeZ;
			}

			leftArm_->SetTranslate(leftLocal);
			rightArm_->SetTranslate(rightLocal);
			leftArmPos_ = leftLocal;
			rightArmPos_ = rightLocal;

			if (bothTelegraphTimer_ >= bothTelegraphDuration_) {
				bothTelegraphActive_ = false;
				bothTelegraphTimer_ = 0.0f;

				leftExtending_ = true;
				rightExtending_ = true;

				leftArmPos_ = leftBothTelegraphTargetPos_;
				rightArmPos_ = rightBothTelegraphTargetPos_;

				leftArm_->SetTranslate(leftArmPos_);
				rightArm_->SetTranslate(rightArmPos_);
			}

			break;
		}

		// =========================================
		// 通常の両手突進
		// =========================================
		if (leftExtending_) {
			leftWorldPos += dirL * bothRushSpeed_;

			float len = MyMath::Length(leftWorldPos - leftBaseWorld);
			bool reachedDist = (len >= maxLen);
			bool hitEnough = (leftArmHitCount_ >= maxHitCount_);

			if (reachedDist || hitEnough) {
				leftExtending_ = false;
			}
		}
		else {
			Vector3 toBase = leftBaseWorld - leftWorldPos;
			float dist = MyMath::Length(toBase);
			if (dist < endThreshold) {
				leftWorldPos = leftBaseWorld;
			}
			else {
				Vector3 dirToBase = MyMath::Normalize(toBase);
				float step = std::min(returnSpeed, dist);
				leftWorldPos += dirToBase * step;
			}
		}

		if (rightExtending_) {
			rightWorldPos += dirR * bothRushSpeed_;

			float len = MyMath::Length(rightWorldPos - rightBaseWorld);
			bool reachedDist = (len >= maxLen);
			bool hitEnough = (rightArmHitCount_ >= maxHitCount_);

			if (reachedDist || hitEnough) {
				rightExtending_ = false;
			}
		}
		else {
			Vector3 toBase = rightBaseWorld - rightWorldPos;
			float dist = MyMath::Length(toBase);
			if (dist < endThreshold) {
				rightWorldPos = rightBaseWorld;
			}
			else {
				Vector3 dirToBase = MyMath::Normalize(toBase);
				float step = std::min(returnSpeed, dist);
				rightWorldPos += dirToBase * step;
			}
		}

		Vector3 leftLocal = leftWorldPos - transform_.translate;
		Vector3 rightLocal = rightWorldPos - transform_.translate;

		leftArm_->SetTranslate(leftLocal);
		rightArm_->SetTranslate(rightLocal);
		leftArmPos_ = leftLocal;
		rightArmPos_ = rightLocal;

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

			bothTelegraphActive_ = false;
			bothTelegraphTimer_ = 0.0f;

			attackPhase_ = AttackPhase::WaitMeteor;
			pendingChargeAfterRetreat_ = true;
			pendingMeteorAfterCharge_ = true;

			armComboActive_ = false;
			armComboFinished_ = true;

			attackPhase_ = AttackPhase::None;
			retreatRequest_ = true;
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
		}
		else if (leftArmPos_.x <= 0.19f) {
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
	}
	else {
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

	// 片手用：最初は予備動作から始める
	isExtending_ = false;
	armTelegraphActive_ = true;
	armTelegraphTimer_ = 0.0f;

	// 両手用
	leftExtending_ = true;
	rightExtending_ = true;

	// ヒット数リセット
	leftArmHitCount_ = 0;
	rightArmHitCount_ = 0;

	// 予備動作用の初期位置
	rightArmPos_ = { 4.0f, 0.0f, 0.0f };
	leftArmPos_ = { -4.0f, 0.0f, 0.0f };

	if (rightArm_) { rightArm_->SetTranslate(rightArmPos_); }
	if (leftArm_) { leftArm_->SetTranslate(leftArmPos_); }
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
	armTelegraphActive_ = false;
	armTelegraphTimer_ = 0.0f;
	bothTelegraphActive_ = false;
	bothTelegraphTimer_ = 0.0f;
	isExtending_ = true;
	leftExtending_ = true;
	rightExtending_ = true;

	// 退避関連もクリア
	retreatActive_ = false;
	retreatRequest_ = false;
	retreatPhase_ = RetreatPhase::None;
	retreatT_ = 0.0f;

	// チャージ停止
	chargeActive_ = false;
	chargeShotLife_ = 0.0f;
	chargeShot_.obj.reset();
	chargeShot_.bullet.reset();

	DeactivateChargeCore();

	if (chargeBeam_) {
		chargeBeam_->Destroy();
	}
}

void BossEnemy::CancelAllAttacks() {

	// 停止処理
	CancelAttacksForMeteor();

	// 退避も止める
	retreatPhase_ = RetreatPhase::None;
	retreatT_ = 0.0f;
	retreatActive_ = false;
	retreatRequest_ = false;
	invulnerable_ = false;

	// 
	DeactivateChargeCore();

	if (chargeBeam_) {
		chargeBeam_->Destroy();
	}

	// 腕を基準位置へ
	leftArmPos_ = { -4.0f, 0.0f, 0.0f };
	rightArmPos_ = { 4.0f, 0.0f, 0.0f };
	if (leftArm_) { leftArm_->SetTranslate(leftArmPos_); }
	if (rightArm_) { rightArm_->SetTranslate(rightArmPos_); }

	// 腕状態リセット
	attackPhase_ = AttackPhase::None;
	armTelegraphActive_ = false;
	armTelegraphTimer_ = 0.0f;
	bothTelegraphActive_ = false;
	bothTelegraphTimer_ = 0.0f;
	armComboActive_ = false;
	armComboFinished_ = false;
	isExtending_ = true;
	leftExtending_ = true;
	rightExtending_ = true;
}

void BossEnemy::StartEnrageTransition(float duration) {

	enrageTransitioning_ = true;
	enragePhase_ = EnrageTransitionPhase::Knockback;
	enrageTransitionTimer_ = 0.0f;
	enrageTransitionDuration_ = duration;
	enrageShockwaveEmitted_ = false;

	// 怒り演出中は無敵
	invulnerable_ = true;

	// 怒り突入時にアーマーを再セット
	ResetArmors(3);

	// いったん全攻撃停止
	CancelAllAttacks();

	// 基準位置保存
	enrageStartPos_ = transform_.translate;

	// duration から各フェーズ時間を組む
	enrageKnockbackDuration_ = std::min(0.25f, duration * 0.18f);
	enrageRecoverDuration_ = std::min(0.35f, duration * 0.18f);
	enrageWaitDuration_ = std::max(0.0f, duration - enrageKnockbackDuration_ - enrageRecoverDuration_);

	// 後方に少し下げる
	enrageKnockbackPos_ = enrageStartPos_;
	enrageKnockbackPos_.z += enrageKnockbackDistance_;
	enrageKnockbackPos_.y += enrageKnockbackLift_;

	// ボスの色をいったん通常へ
	if (object3d_) {
		object3d_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
	if (leftArm_) {
		leftArm_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
	if (rightArm_) {
		rightArm_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
}

void BossEnemy::UpdateEnrageTransition(float dt) {

	if (!enrageTransitioning_) {
		return;
	}

	enrageTransitionTimer_ += dt;

	auto lerp3 = [](const Vector3& a, const Vector3& b, float t) {
		return Vector3{
			a.x + (b.x - a.x) * t,
			a.y + (b.y - a.y) * t,
			a.z + (b.z - a.z) * t,
		};
		};

	const Vector4 baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	const Vector4 redColor = { 1.0f, 0.25f, 0.25f, 1.0f };

	switch (enragePhase_) {

	case EnrageTransitionPhase::Knockback:
	{
		float t = (enrageKnockbackDuration_ > 0.0f)
			? (enrageTransitionTimer_ / enrageKnockbackDuration_)
			: 1.0f;
		t = std::clamp(t, 0.0f, 1.0f);

		// 勢いよく飛ぶ
		float ease = 1.0f - (1.0f - t) * (1.0f - t);

		transform_.translate = lerp3(enrageStartPos_, enrageKnockbackPos_, ease);

		if (t >= 1.0f) {
			enragePhase_ = EnrageTransitionPhase::Wait;
			enrageTransitionTimer_ = 0.0f;
		}
		break;
	}

	case EnrageTransitionPhase::Wait:
	{
		// 基本停止位置
		Vector3 pos = enrageKnockbackPos_;

		// 小刻みシェイク
		float sx = std::sin(enrageTransitionTimer_ * enrageShakeFrequency_) * enrageShakeAmplitude_;
		float sz = std::cos(enrageTransitionTimer_ * (enrageShakeFrequency_ * 1.27f)) * enrageShakeAmplitude_;
		pos.x += sx;
		pos.z += sz;
		transform_.translate = pos;

		// 赤フラッシュ
		float flash = (std::sin(enrageTransitionTimer_ * enrageFlashSpeed_) + 1.0f) * 0.5f;
		Vector4 c{
			baseColor.x + (redColor.x - baseColor.x) * flash,
			baseColor.y + (redColor.y - baseColor.y) * flash,
			baseColor.z + (redColor.z - baseColor.z) * flash,
			1.0f
		};

		if (object3d_) { object3d_->SetColor(c); }
		if (leftArm_) { leftArm_->SetColor(c); }
		if (rightArm_) { rightArm_->SetColor(c); }

		if (enrageTransitionTimer_ >= enrageWaitDuration_) {
			enragePhase_ = EnrageTransitionPhase::Recover;
			enrageTransitionTimer_ = 0.0f;

			// 復帰開始時に衝撃波
			if (!enrageShockwaveEmitted_) {
				auto* pm = KomEngine::System::GetParticleManager();
				if (pm) {
					if (pm->Exists("ring")) {
						pm->Emit("ring", transform_.translate, 1);
					}
					if (pm->Exists("dust")) {
						pm->Emit("dust", transform_.translate, 18);
					}
				}
				if (camera_) {
					camera_->StartShake(CameraShakeType::Large);
				}
				enrageShockwaveEmitted_ = true;
			}
		}
		break;
	}

	case EnrageTransitionPhase::Recover:
	{
		float t = (enrageRecoverDuration_ > 0.0f)
			? (enrageTransitionTimer_ / enrageRecoverDuration_)
			: 1.0f;
		t = std::clamp(t, 0.0f, 1.0f);

		// 少しゆっくり戻す
		float ease = t * t * (3.0f - 2.0f * t);

		transform_.translate = lerp3(enrageKnockbackPos_, enrageStartPos_, ease);

		// 色を戻す
		if (object3d_) { object3d_->SetColor(baseColor); }
		if (leftArm_) { leftArm_->SetColor(baseColor); }
		if (rightArm_) { rightArm_->SetColor(baseColor); }

		if (t >= 1.0f) {
			transform_.translate = enrageStartPos_;
			enragePhase_ = EnrageTransitionPhase::None;
			enrageTransitioning_ = false;
			enrageTransitionTimer_ = 0.0f;
			enrageShockwaveEmitted_ = false;

			// 怒り演出終了で無敵解除
			invulnerable_ = false;
		}
		break;
	}

	default:
		enragePhase_ = EnrageTransitionPhase::None;
		enrageTransitioning_ = false;
		enrageTransitionTimer_ = 0.0f;
		enrageShockwaveEmitted_ = false;
		break;
	}
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

	// いきなり伸ばさず、予備動作から
	isExtending_ = false;
	armTelegraphActive_ = true;
	armTelegraphTimer_ = 0.0f;

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

void BossEnemy::SetRotate(const Vector3& rotate) {
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

	// 無敵中は本体ダメージを受けない
	if (invulnerable_ || enrageTransitioning_) {
		return;
	}

	// 被弾シェイク開始
	StartBodyHitShake();

	// 胴体フラッシュ開始
	StartBodyHitFlash();

	// まだアーマーが残っているなら、本体ではなくアーマーにダメージ
	if (!AreAllArmorsBroken()) {
		DamageArmor(v);
		return;
	}

	// ---------------- 本体ダメージ ---------------- //

	float prevRatio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
	prevRatio = std::clamp(prevRatio, 0.0f, 1.0f);
	float prevWidth = 700.0f * prevRatio;

	hp_ = std::max(0, hp_ - v);

	float newRatio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
	newRatio = std::clamp(newRatio, 0.0f, 1.0f);
	float newWidth = 700.0f * newRatio;

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

		if (deathPhase_ == DeathPhase::PreFall) {
			float sx = std::sin(fallShakeTime_ * 85.0f) * 0.12f;
			float sy = std::cos(fallShakeTime_ * 110.0f) * 0.06f;
			float sz = std::cos(fallShakeTime_ * 95.0f) * 0.12f;
			bodyPos.x += sx;
			bodyPos.y += sy;
			bodyPos.z += sz;
		}
		else if (deathPhase_ == DeathPhase::FinalExplosion) {
			float sx = std::sin(fallShakeTime_ * 45.0f) * 0.25f;
			float sz = std::cos(fallShakeTime_ * 52.0f) * 0.25f;
			bodyPos.x += sx;
			bodyPos.z += sz;
		}
		else {
			float sx = std::sin(fallShakeTime_ * 40.0f) * fallShakeAmplitude_;
			float sz = std::cos(fallShakeTime_ * 55.0f) * fallShakeAmplitude_;
			bodyPos.x += sx;
			bodyPos.z += sz;
		}
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

	// 怒り遷移の待機中は赤点滅を優先
	if (enrageTransitioning_ && enragePhase_ == EnrageTransitionPhase::Wait) {
		float flash = (std::sin(enrageTransitionTimer_ * enrageFlashSpeed_) + 1.0f) * 0.5f;
		Vector4 rageColor{
			baseColor.x + (flashColor.x - baseColor.x) * flash,
			baseColor.y + (flashColor.y - baseColor.y) * flash,
			baseColor.z + (flashColor.z - baseColor.z) * flash,
			1.0f
		};

		if (object3d_) { object3d_->SetColor(rageColor); }
		if (leftArm_) { leftArm_->SetColor(rageColor); }
		if (rightArm_) { rightArm_->SetColor(rageColor); }
		return;
	}

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

	if (hp_ <= 0) return;
	retreatRequest_ = true;
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

		}
		else { // Hold

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
		m.obj->SetModel("BossEnemyMissile.obj");
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
			float t = (float)i / 3.0f;   // 0, 1/3, 2/3, 1
			float rad = t * 3.14159265f; // 0..π

			// 半円を X-Y 平面に
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
				break;
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

	useLeftArm; // 今回は使わない

	if (!chargeBeam_ || chargeBeam_->IsActive()) {
		return;
	}
	if (!player_) {
		return;
	}

	const Vector3 spawnPos = GetChargeCoreWorldPos();
	const Vector3 playerPos = player_->GetTransform().translate;
	const Vector3 dir = MyMath::Normalize(playerPos - spawnPos);

	chargeBeam_->Fire(spawnPos, dir);
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

	if (enrageTransitioning_) {
		return;
	}

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

	auto* pm = KomEngine::System::GetParticleManager();
	if (!pm) {
		return;
	}

	if (chargeActive_) {

		Vector3 fxPos = GetChargeCoreWorldPos();

		const bool hasCore = pm->Exists("charge_core");
		const bool hasPulse = pm->Exists("charge_pulse");
		const bool hasMoon = pm->Exists("moonLight");
		const bool hasAura = pm->Exists("charge_aura");
		const bool hasLine = pm->Exists("player_charge_line");

		// 青白い気の色に統一
		pm->SetChargeEffectColor(
			{ 0.72f, 0.90f, 1.00f, 1.0f },   // core
			{ 0.85f, 0.95f, 1.00f, 1.0f }    // pulse
		);

		// 中心に吸い込まれる細かい粒
		chargeFxCoreTimer_ += dt;
		if (chargeFxCoreTimer_ >= 0.035f) {
			chargeFxCoreTimer_ = 0.0f;

			if (hasCore) {
				pm->Emit("charge_core", fxPos, 12);
			}
		}

		// 遠くから中心に集まる線
		chargeFxRibbonTimer_ += dt;
		if (chargeFxRibbonTimer_ >= 0.060f) {
			chargeFxRibbonTimer_ = 0.0f;

			if (hasLine) {
				pm->Emit("player_charge_line", fxPos, 5);
			}
		}

		// 外周の脈動リング
		chargeFxPulseTimer_ += dt;
		if (chargeFxPulseTimer_ >= 0.22f) {
			chargeFxPulseTimer_ = 0.0f;

			if (hasPulse) {
				pm->Emit("charge_pulse", fxPos, 1);
			}
		}

		// たまに十字っぽい光を足す
		// moonLight はチャージ演出では一旦使わない
		chargeFxRingTimer_ = 0.0f;

		// 中心の大きい“気の塊”本体
		chargeFxCylinderTimer_ += dt;
		if (chargeFxCylinderTimer_ >= 0.18f) {
			chargeFxCylinderTimer_ = 0.0f;

			if (hasAura) {
				pm->Emit("charge_aura", fxPos, 2);
			}
		}
	}
	else {
		chargeFxCoreTimer_ = 0.0f;
		chargeFxPulseTimer_ = 0.0f;
		chargeFxRibbonTimer_ = 0.0f;
		chargeFxRingTimer_ = 0.0f;
		chargeFxCylinderTimer_ = 0.0f;
	}
}

void BossEnemy::StartDeathEffect() {

	deathEffectStarted_ = true;
	deathPhase_ = DeathPhase::PreFall;

	deathEffectTimer_ = 0.0f;
	finalExplosionTimer_ = 0.0f;
	deathSparkTimer_ = 0.0f;

	finalExplosionDone_ = false;

	fallStarted_ = true;
	fallVelY_ = 0.0f;
	fallRotateStart_ = transform_.rotate.x;
	fallShakeTime_ = 0.0f;

	combatEnabled_ = false;
	isAttack_ = false;
	invulnerable_ = true;

	CancelAllAttacks();
}

void BossEnemy::UpdateDeathEffect(float dt) {

	// -----------------------------
	// ビリビリ演出
	// -----------------------------
	if (deathPhase_ == DeathPhase::PreFall) {

		deathEffectTimer_ += dt;
		deathSparkTimer_ += dt;
		fallShakeTime_ += dt;

		if (deathSparkTimer_ >= deathSparkInterval_) {
			deathSparkTimer_ = 0.0f;
			EmitDeathElectricParticles();
		}

		// たまに軽い火花
		if (std::fmod(deathEffectTimer_, 0.22f) < dt) {
			auto* pm = KomEngine::System::GetParticleManager();
			if (pm && pm->Exists("hit")) {
				pm->Emit("hit", transform_.translate, 6);
			}
		}

		if (deathEffectTimer_ >= deathEffectDuration_) {
			deathPhase_ = DeathPhase::FinalExplosion;
			finalExplosionTimer_ = 0.0f;
			TriggerFinalExplosion();
		}

		return;
	}

	// -----------------------------
	// 大爆発を少し見せる
	// -----------------------------
	if (deathPhase_ == DeathPhase::FinalExplosion) {

		finalExplosionTimer_ += dt;
		fallShakeTime_ += dt;

		if (finalExplosionTimer_ >= finalExplosionDuration_) {
			deathPhase_ = DeathPhase::Falling;
		}
		return;
	}

	// -----------------------------
	// 落下
	// -----------------------------
	if (deathPhase_ == DeathPhase::Falling && !hasLanded_) {

		fallShakeTime_ += dt;

		fallVelY_ += gravityY_;
		transform_.translate.y += fallVelY_;

		float fallProgress = (transform_.translate.y - groundY_) / (2.0f - groundY_);
		fallProgress = std::clamp(1.0f - fallProgress, 0.0f, 1.0f);

		float ease = fallProgress * fallProgress;
		transform_.rotate.x = MyMath::Lerp(fallRotateStart_, fallRotateEnd_, ease);

		if (transform_.translate.y <= groundY_) {
			transform_.translate.y = groundY_;
			fallVelY_ = 0.0f;
			hasLanded_ = true;
			deathPhase_ = DeathPhase::Landed;

			if (!landingShakeDone_ && camera_) {
				camera_->StartShake(CameraShakeType::Large);
				landingShakeDone_ = true;
			}

			auto* pm = KomEngine::System::GetParticleManager();
			if (pm && pm->Exists("dust")) {
				pm->Emit("dust", transform_.translate, 120);
			}
		}
	}
}

void BossEnemy::EmitDeathElectricParticles() {

	auto* pm = KomEngine::System::GetParticleManager();
	if (!pm) {
		return;
	}

	// 本体の周囲ランダム位置
	Vector3 p{
		transform_.translate.x + MyMath::Rand(-2.2f, 2.2f),
		transform_.translate.y + MyMath::Rand(-1.2f, 2.0f),
		transform_.translate.z + MyMath::Rand(-2.2f, 2.2f)
	};

	// 今ある粒子で代用
	if (pm->Exists("hit")) {
		pm->Emit("hit", p, 4);
	}

	// たまに少し強めの火花
	if (pm->Exists("explosion") && MyMath::Rand(0.0f, 1.0f) < 0.25f) {
		pm->Emit("explosion", p, 2);
	}
}

void BossEnemy::TriggerFinalExplosion() {

	if (finalExplosionDone_) {
		return;
	}
	finalExplosionDone_ = true;

	auto* pm = KomEngine::System::GetParticleManager();
	if (pm) {
		// 爆心
		if (pm->Exists("explosion")) {
			pm->Emit("explosion", transform_.translate, 120);
		}

		// 火花を強めに
		if (pm->Exists("hit")) {
			pm->Emit("hit", transform_.translate, 60);
		}

		//// 衝撃波リング
		//if (pm->Exists("ring")) {
		//	pm->Emit("ring", transform_.translate, 2);
		//}

		//// 爆発余韻
		//if (pm->Exists("dust")) {
		//	pm->Emit("dust", transform_.translate, 35);
		//}
	}

	if (camera_) {
		camera_->StartShake(CameraShakeType::Large);
	}
}

// ----------------------- Armor（装甲） ----------------------- //

int BossEnemy::GetAliveArmorCount() const {

	int c = 0;
	for (const auto& a : armors_) {
		if (a.alive) { ++c; }
	}
	return c;
}

void BossEnemy::InitArmors() {

	armors_.clear();
	armors_.reserve(armorInitialCount_);

	armorTime_ = 0.0f;
	armorGlobalAngle_ = 0.0f;

	for (int i = 0; i < armorInitialCount_; ++i) {
		ArmorUnit u{};
		u.obj = std::make_unique<Object3d>();
		u.obj->Init(BlendType::BLEND_NONE);
		u.obj->SetModel(kBossArmorModel);
		u.obj->SetDefaultCamera(camera_);
		u.obj->SetParent(object3d_.get());

		// 見た目サイズ
		u.obj->SetScale(armorScale_);

		float t = (armorInitialCount_ > 0) ? (float)i / (float)armorInitialCount_ : 0.0f;
		u.angle = MyMath::GetPI() * 2.0f * t;

		u.alive = true;
		u.hp = 3;

		armors_.push_back(std::move(u));
	}
}

bool BossEnemy::DamageArmor(int damage) {

	if (damage <= 0) {
		return false;
	}

	// 生きているアーマーのうち、末尾側から1個選んでダメージ
	for (int i = static_cast<int>(armors_.size()) - 1; i >= 0; --i) {
		auto& a = armors_[i];
		if (!a.alive) {
			continue;
		}

		a.hp -= damage;

		if (a.hp <= 0) {
			a.hp = 0;

			// 壊れる直前の位置を取る
			Vector3 breakPos = transform_.translate;
			if (a.obj) {
				breakPos = a.obj->GetWorldPosition();
			}

			a.alive = false;

			// 軽い爆発エフェクト
			auto* pm = KomEngine::System::GetParticleManager();
			if (pm) {
				if (pm->Exists("explosion")) {
					pm->Emit("explosion", breakPos, 12);
				}
				if (pm->Exists("hit")) {
					pm->Emit("hit", breakPos, 18);
				}
			}
		}

		return true;
	}

	return false;
}

void BossEnemy::BreakOneArmor() {

	// すでに全部壊れてるなら何もしない
	if (GetAliveArmorCount() <= 0) { return; }

	// 末尾側から壊す（見た目が一定になって分かりやすい）
	for (int i = (int)armors_.size() - 1; i >= 0; --i) {
		auto& a = armors_[i];
		if (a.alive) {
			a.alive = false;
			return;
		}
	}
}

void BossEnemy::UpdateArmors(float dt) {

	armorTime_ += dt;

	// alive のインデックスを集める
	std::vector<int> aliveIdx;
	aliveIdx.reserve(armors_.size());
	for (int i = 0; i < (int)armors_.size(); ++i) {
		if (armors_[i].obj && armors_[i].alive) {
			aliveIdx.push_back(i);
		}
	}

	const int n = (int)aliveIdx.size();
	if (n <= 0) { return; }

	// 全体回転
	armorGlobalAngle_ += armorOrbitSpeed_ * dt;

	// n 等分
	const float step = (MyMath::GetPI() * 2.0f) / (float)n;

	for (int order = 0; order < n; ++order) {

		auto& a = armors_[aliveIdx[order]];
		if (!a.obj) { continue; }

		const float ang = armorGlobalAngle_ + step * (float)order;

		float y = std::sinf(armorTime_ * armorFloatSpeed_ + ang) * armorFloatAmp_;

		Vector3 local{};
		local.x = std::cosf(ang) * armorOrbitRadius_;
		local.y = y;
		local.z = std::sinf(ang) * armorOrbitRadius_;

		a.obj->SetTranslate(local);

		// 
		a.obj->SetScale(armorScale_);

		a.obj->Update();
	}
}

void BossEnemy::DrawArmors() {

	for (auto& a : armors_) {
		if (a.obj && a.alive) {
			a.obj->Draw();
		}
	}
}

void BossEnemy::ResetArmors(int hp) {

	if (hp <= 0) {
		hp = 1;
	}

	// 個数が変わっていたら作り直す
	if (static_cast<int>(armors_.size()) != armorInitialCount_) {
		InitArmors();
	}

	armorTime_ = 0.0f;
	armorGlobalAngle_ = 0.0f;

	const int count = static_cast<int>(armors_.size());
	for (int i = 0; i < count; ++i) {
		auto& a = armors_[i];

		a.alive = true;
		a.hp = hp;

		float t = (count > 0) ? static_cast<float>(i) / static_cast<float>(count) : 0.0f;
		a.angle = MyMath::GetPI() * 2.0f * t;

		if (a.obj) {
			a.obj->SetScale(armorScale_);
			a.obj->SetParent(object3d_.get());
		}
	}
}

bool BossEnemy::AreAllArmorsBroken() const {

	for (const auto& a : armors_) {
		if (a.alive) {
			return false;
		}
	}
	return true;
}

bool BossEnemy::ConsumeRetreatRequest() {

	if (!retreatRequest_) {
		return false;
	}
	retreatRequest_ = false;
	return true;
}

void BossEnemy::ApplyRetreatPose(const Vector3& pos, const Vector3& bodyScale, const Vector3& armScale) {

	transform_.translate = pos;

	retreatVisualOverride_ = true;
	retreatBodyScale_ = bodyScale;
	retreatArmScale_ = armScale;
}

void BossEnemy::ClearRetreatVisualOverride() {

	retreatVisualOverride_ = false;
	retreatBodyScale_ = baseBodyScale_;
	retreatArmScale_ = baseArmScale_;
}

void BossEnemy::ActivateChargeCore() {

	if (!chargeCore_) {
		return;
	}

	chargeCore_->ResetHP(chargeCoreHp_);
	chargeCore_->Activate(transform_.translate + chargeCoreOffset_);
}

void BossEnemy::DeactivateChargeCore() {

	if (chargeCore_) {
		chargeCore_->Deactivate();
	}
}

bool BossEnemy::IsChargeCoreBroken() const {

	return chargeCore_ && chargeCore_->IsActive() && chargeCore_->IsBroken();
}

bool BossEnemy::IsChargeCoreActive() const {

	return chargeCore_ && chargeCore_->IsActive();
}

Vector3 BossEnemy::GetChargeCoreWorldPos() const {

	if (chargeCore_ && chargeCore_->IsActive()) {
		return chargeCore_->GetWorldPos();
	}
	return transform_.translate + chargeCoreOffset_;
}

bool BossEnemy::IsMissileTelegraphing() const {

	return missilePhase_ == MissilePhase::Telegraph;
}

bool BossEnemy::GetMissileTelegraphWorldPos(int index, Vector3& outPos) const {

	if (index < 0 || index >= static_cast<int>(missiles_.size())) {
		return false;
	}

	const auto& m = missiles_[index];
	if (!m.bullet) {
		return false;
	}

	outPos = m.bullet->GetTranslate();
	return true;
}