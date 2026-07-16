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

// ============================================================
// 腕攻撃State
// ============================================================

class BossEnemy::SingleArmAttackState : public BossEnemy::ArmAttackState {

public:

	explicit SingleArmAttackState(AttackPhase phase)
		: phase_(phase) {
	}

	void Enter(BossEnemy& boss) override {

		// 右腕か左腕のどちらか以外では使わない
		boss.attackPhase_ = phase_;
	}

	void Update(BossEnemy& boss, float dt) override {

		boss.UpdateSingleArmAttack(dt);
	}

	AttackPhase GetPhase() const override {

		return phase_;
	}

private:

	AttackPhase phase_ = AttackPhase::None;
};

class BossEnemy::BothHandsAttackState : public BossEnemy::ArmAttackState {

public:

	void Enter(BossEnemy& boss) override {

		boss.attackPhase_ = AttackPhase::BothHands;
	}

	void Update(BossEnemy& boss, float dt) override {

		boss.UpdateBothHandsAttack(dt);
	}

	AttackPhase GetPhase() const override {

		return AttackPhase::BothHands;
	}
};

class BossEnemy::WaitMeteorAttackState : public BossEnemy::ArmAttackState {

public:

	void Enter(BossEnemy& boss) override {

		boss.attackPhase_ = AttackPhase::WaitMeteor;
	}

	void Update(BossEnemy& boss, float dt) override {

		(void)dt;
		boss.UpdateWaitMeteorAttack();
	}

	AttackPhase GetPhase() const override {

		return AttackPhase::WaitMeteor;
	}
};

BossEnemy::~BossEnemy() = default;

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

	// ボスのHP
	hpUI_ = std::make_unique<BossHpUI>();
	hpUI_->Init();

	// 撃破演出
	deathController_ = std::make_unique<BossDeathController>();

	// 装甲管理
	armorController_ = std::make_unique<BossArmorController>();
	armorController_->Init(camera_, object3d_.get());

	// チャージコア、チャージビームの生成
	chargeCore_ = std::make_unique<BossChargeCore>();
	chargeCore_->Init(camera_);

	chargeBeam_ = std::make_unique<BossChargeBeam>();
	chargeBeam_->Init(camera_);

	// チャージ中のパーティクル演出
	chargeEffectController_ = std::make_unique<BossChargeEffectController>();

	// 怒り遷移演出
	enrageController_ = std::make_unique<BossEnrageTransitionController>();

	// 腕攻撃の挙動管理
	armAttackController_ = std::make_unique<BossArmAttackController>();

	// 当たり判定コライダーの設定
	bodyCol_.owner = this;
	bodyCol_.part = PartCollider::Part::Body;

	leftCol_.owner = this;
	leftCol_.part = PartCollider::Part::LeftArm;

	rightCol_.owner = this;
	rightCol_.part = PartCollider::Part::RightArm;

	// スタン中の星演出
	dizzyStarController_ = std::make_unique<BossDizzyStarController>();
	dizzyStarController_->Init(camera_);

	// --- 怒り用：通常時の基準値を保存 ---
	baseAttackSpeed_ = attackSpeed_;
	baseArmReturnSpeedSingle_ = armReturnSpeedSingle_;
	baseArmReturnSpeedBoth_ = armReturnSpeedBoth_;
}

void BossEnemy::Update() {

	const float dt = KomEngine::System::GetDeltaTime();

	// チャージ・怒り・コア破壊など、状態に関係なく必要な演出更新
	UpdateCommonEffects(dt);

	// 3Dオブジェクト、装甲、スタン星などの見た目更新
	UpdateBossObjects(dt);

	// 被弾シェイクなどのタイマー更新
	UpdateDamageTimers(dt);

	// HPバー・HPチップ更新
	UpdateHpUI(dt);

	// 生存中 / 撃破後で処理を分ける
	if (hp_ <= 0) {
		UpdateDead(dt);
	}
	else {
		UpdateAlive(dt);
	}

	// 生存中だけチャージビーム弾を更新
	if (hp_ > 0) {
		UpdateChargeBeamShot(dt);
	}

	// 被弾・落下シェイクを反映
	DamageShake();

	// 腕の表示状態を更新
	UpdateArmVisibility();

	// スケールと当たり判定半径を更新
	UpdateVisualScaleAndCollisionRadius();
}

void BossEnemy::Draw() {

	//
	object3d_->Draw();

	// 装甲
	if (armorController_) {
		armorController_->Draw();
	}

	// スタン中の星
	if (dizzyStarController_) {
		dizzyStarController_->Draw();
	}

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

void BossEnemy::HPDraw() {

	if (hpUI_) {
		hpUI_->Draw();
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

	ImGui::Separator();

	if (hpUI_) {
		hpUI_->ImGuiDebug();
	}

	ImGui::Checkbox("攻撃中", &isAttack_);
	ImGui::Checkbox("怒り状態", &isEnraged_);

	// 装甲のデバッグ表示
	if (armorController_) {
		armorController_->ImGuiDebug();
	}

	ImGui::End();

#endif
}

void BossEnemy::UpdateCommonEffects(float dt) {

	UpdateChargeCrossPose(dt);

	if (enrageController_) {
		enrageController_->Update(
			dt,
			transform_,
			object3d_.get(),
			leftArm_.get(),
			rightArm_.get(),
			camera_,
			isEnraged_
		);

		if (enrageController_->ConsumeFinished()) {
			invulnerable_ = false;
		}
	}

	if (chargeEffectController_) {
		chargeEffectController_->Update(
			dt,
			chargeActive_,
			GetChargeCoreWorldPos()
		);
	}

	if (chargeCore_ && chargeCore_->IsActive()) {
		chargeCore_->SetWorldPos(transform_.translate + chargeCoreOffset_);
		chargeCore_->Update(dt);
	}

	if (chargeCore_ && chargeCore_->ConsumeBrokenJustNow()) {
		StartCoreBreakReaction();
	}

	UpdateCoreBreakReaction(dt);

	if (chargeBeam_ && chargeBeam_->IsActive()) {
		chargeBeam_->Update(dt);
	}
}

void BossEnemy::UpdateBossObjects(float dt) {

	if (object3d_) {
		object3d_->Update();
	}

	if (leftArm_) {
		leftArm_->Update();
	}

	if (rightArm_) {
		rightArm_->Update();
	}

	// スタン中の星演出更新
	if (dizzyStarController_) {
		dizzyStarController_->Update(dt, transform_.translate);
	}

	// 装甲（周回）更新
	if (armorController_) {
		armorController_->Update(dt);
	}
}

void BossEnemy::UpdateDamageTimers(float dt) {

	auto updateTimer = [dt](float& timer) {
		if (timer > 0.0f) {
			timer -= dt;
			if (timer < 0.0f) {
				timer = 0.0f;
			}
		}
		};

	updateTimer(bodyHitShakeTime_);
	updateTimer(leftHitShakeTime_);
	updateTimer(rightHitShakeTime_);
}

void BossEnemy::UpdateHpUI(float dt) {

	if (hpUI_) {
		hpUI_->Update(hp_, maxHp_, dt);
	}
}

void BossEnemy::UpdateDead(float dt) {

	if (!deathController_) {
		return;
	}

	// 撃破演出の更新
	if (!deathController_->IsStarted()) {

		combatEnabled_ = false;
		isAttack_ = false;
		invulnerable_ = true;

		CancelAllAttacks();

		deathController_->Start(transform_);
	}

	if (!deathController_->HasLanded()) {
		deathController_->Update(dt, transform_);
	}

	if (deathController_->ConsumeFinalExplosionShakeRequest()) {
		if (camera_) {
			camera_->StartShake(CameraShakeType::Large);
		}
	}

	if (deathController_->ConsumeLandingShakeRequest()) {
		if (camera_) {
			camera_->StartShake(CameraShakeType::Large);
		}
	}
}

void BossEnemy::UpdateAlive(float dt) {

	(void)dt;

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

void BossEnemy::UpdateArmVisibility() {

	bool showLeft = false;
	bool showRight = false;

	if (armComboActive_) {
		switch (attackPhase_) {
		case AttackPhase::SingleLeft:
			showLeft = true;
			break;

		case AttackPhase::SingleRight:
			showRight = true;
			break;

		case AttackPhase::BothHands:
			showLeft = true;
			showRight = true;
			break;

		default:
			break;
		}
	}

	leftArmVisible_ = showLeft;
	rightArmVisible_ = showRight;
}

void BossEnemy::UpdateVisualScaleAndCollisionRadius() {

	const Vector3 useBodyScale =
		retreatVisualOverride_ ? retreatBodyScale_ : baseBodyScale_;

	const Vector3 useArmScale =
		retreatVisualOverride_ ? retreatArmScale_ : baseArmScale_;

	if (object3d_) {
		object3d_->SetScale(useBodyScale);
		object3d_->SetRadius(bodyRadius_);
	}

	if (leftArm_) {
		leftArm_->SetScale(leftArmVisible_ ? useArmScale : Vector3{ 0.0f, 0.0f, 0.0f });
		leftArm_->SetRadius(leftArmVisible_ ? (leftArmRadius_ * leftArm_->GetScale().x) : 0.0f);
	}

	if (rightArm_) {
		rightArm_->SetScale(rightArmVisible_ ? useArmScale : Vector3{ 0.0f, 0.0f, 0.0f });
		rightArm_->SetRadius(rightArmVisible_ ? (rightArmRadius_ * rightArm_->GetScale().x) : 0.0f);
	}
}

void BossEnemy::Attack() {

	if (!CanUpdateAttack()) {
		return;
	}

	if (!armAttackState_) {
		return;
	}

	const float dt = KomEngine::System::GetDeltaTime();

	armAttackState_->Update(*this, dt);
}

bool BossEnemy::CanUpdateAttack() const {

	if (!player_) {
		return false;
	}

	// 怒り遷移中は攻撃しない
	if (IsEnrageTransitioning()) {
		return false;
	}

	if (!armComboActive_) {
		return false;
	}

	return true;
}

void BossEnemy::ChangeArmAttackState(std::unique_ptr<ArmAttackState> nextState) {

	if (!nextState) {
		armAttackState_.reset();
		attackPhase_ = AttackPhase::None;
		return;
	}

	attackPhase_ = nextState->GetPhase();

	armAttackState_ = std::move(nextState);
	armAttackState_->Enter(*this);
}

void BossEnemy::StartSingleArmPhase(AttackPhase phase) {

	if (phase != AttackPhase::SingleLeft &&
		phase != AttackPhase::SingleRight) {
		return;
	}

	armComboActive_ = true;

	// 片腕攻撃は予備動作から開始する
	isExtending_ = false;

	armTelegraphActive_ = true;
	armTelegraphTimer_ = 0.0f;

	bothTelegraphActive_ = false;
	bothTelegraphTimer_ = 0.0f;

	armWindSlashFxTimer_ = 0.0f;

	ChangeArmAttackState(std::make_unique<SingleArmAttackState>(phase));
}

void BossEnemy::StartBothHandsPhase() {

	armComboActive_ = true;

	// 片腕用の予備動作は終了
	armTelegraphActive_ = false;
	armTelegraphTimer_ = 0.0f;

	// 両手用の予備動作から開始
	bothTelegraphActive_ = true;
	bothTelegraphTimer_ = 0.0f;

	leftExtending_ = false;
	rightExtending_ = false;

	leftArmPos_ = { -4.0f, 0.0f, 0.0f };
	rightArmPos_ = { 4.0f, 0.0f, 0.0f };

	if (leftArm_) {
		leftArm_->SetTranslate(leftArmPos_);
	}
	if (rightArm_) {
		rightArm_->SetTranslate(rightArmPos_);
	}

	armWindSlashFxTimer_ = 0.0f;

	ChangeArmAttackState(std::make_unique<BothHandsAttackState>());
}

void BossEnemy::FinishArmCombo() {

	leftArmHitCount_ = 0;
	rightArmHitCount_ = 0;

	leftExtending_ = true;
	rightExtending_ = true;

	armTelegraphActive_ = false;
	armTelegraphTimer_ = 0.0f;

	bothTelegraphActive_ = false;
	bothTelegraphTimer_ = 0.0f;

	pendingChargeAfterRetreat_ = true;
	pendingMeteorAfterCharge_ = true;

	armComboActive_ = false;
	armComboFinished_ = true;

	retreatRequest_ = true;

	ChangeArmAttackState(nullptr);
}

void BossEnemy::ResetArmAttackState() {

	armComboActive_ = false;
	armComboFinished_ = false;

	armTelegraphActive_ = false;
	armTelegraphTimer_ = 0.0f;

	bothTelegraphActive_ = false;
	bothTelegraphTimer_ = 0.0f;

	isExtending_ = true;
	leftExtending_ = true;
	rightExtending_ = true;

	armWindSlashFxTimer_ = 0.0f;

	ChangeArmAttackState(nullptr);
}

void BossEnemy::UpdateSingleArmAttack(float dt) {

	if (!armAttackController_) {
		return;
	}

	armAttackController_->UpdateSingleArm(
		*this,
		dt
	);
}

void BossEnemy::UpdateBothHandsAttack(float dt) {

	if (!armAttackController_) {
		return;
	}

	armAttackController_->UpdateBothHands(
		*this,
		dt
	);
}

void BossEnemy::UpdateWaitMeteorAttack() {

	// Scene側 / AttackManager側でメテオを出している間は腕攻撃しない
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
	if (armComboActive_) {
		return;
	}

	armComboActive_ = true;
	armComboFinished_ = false;

	// ヒット数リセット
	leftArmHitCount_ = 0;
	rightArmHitCount_ = 0;

	// 腕の初期位置
	rightArmPos_ = { 4.0f, 0.0f, 0.0f };
	leftArmPos_ = { -4.0f, 0.0f, 0.0f };

	if (rightArm_) {
		rightArm_->SetTranslate(rightArmPos_);
	}
	if (leftArm_) {
		leftArm_->SetTranslate(leftArmPos_);
	}

	// コンボ開始は「右 → 左 → 両手」
	StartSingleArmPhase(AttackPhase::SingleRight);
}

bool BossEnemy::ConsumeArmComboFinished() {

	if (!armComboFinished_) return false;
	armComboFinished_ = false;
	return true;
}

void BossEnemy::CancelAttacksForMeteor() {

	// 腕攻撃State停止
	ResetArmAttackState();

	// 退避関連もクリア
	retreatActive_ = false;
	retreatRequest_ = false;
	retreatVisualOverride_ = false;
	invulnerable_ = false;

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
	retreatActive_ = false;
	retreatRequest_ = false;
	retreatVisualOverride_ = false;
	retreatBodyScale_ = baseBodyScale_;
	retreatArmScale_ = baseArmScale_;
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

	// 腕攻撃Stateをリセット
	ResetArmAttackState();
}

void BossEnemy::StartEnrageTransition(float duration) {

	// いったん全攻撃停止
	CancelAllAttacks();

	// 怒り演出中は無敵
	invulnerable_ = true;

	// 怒り突入時にアーマーを再セット
	if (armorController_) {
		armorController_->Reset(3);
	}

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

	if (enrageController_) {
		enrageController_->Start(duration, transform_.translate);
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

void BossEnemy::SetEnraged(bool enraged) {

	if (isEnraged_ == enraged) {
		return;
	}

	isEnraged_ = enraged;

	if (isEnraged_) {
		attackSpeed_ = baseAttackSpeed_ * enragedArmSpeedMul_;
		armReturnSpeedSingle_ = baseArmReturnSpeedSingle_ * enragedArmSpeedMul_;
		armReturnSpeedBoth_ = baseArmReturnSpeedBoth_ * enragedArmSpeedMul_;
	}
	else {
		attackSpeed_ = baseAttackSpeed_;
		armReturnSpeedSingle_ = baseArmReturnSpeedSingle_;
		armReturnSpeedBoth_ = baseArmReturnSpeedBoth_;

		if (object3d_) { object3d_->SetModel("BossEnemyCore.obj"); }
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
	if (invulnerable_ || IsEnrageTransitioning()) {
		return;
	}

	// 被弾シェイク開始
	StartBodyHitShake();

	// まだアーマーが残っているなら、本体ではなくアーマーにダメージ
	if (armorController_ && !armorController_->AreAllBroken()) {
		armorController_->Damage(v, transform_.translate);
		return;
	}

	// ----------------本体ダメージ---------------- //

	int prevHp = hp_;

	hp_ = std::max(0, hp_ - v);

	// 減った部分からHPチップを出す
	if (hpUI_) {
		hpUI_->OnHpChanged(prevHp, hp_, maxHp_);
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
		break;

	case Part::LeftArm:
		// 腕は「ヒット数」で壊れる仕様なので、dmg分ヒットを加算
		for (int i = 0; i < dmg; ++i) {
			owner->AddHitLeftArm();
		}
		owner->StartLeftArmHitShake();
		break;

	case Part::RightArm:
		for (int i = 0; i < dmg; ++i) {
			owner->AddHitRightArm();
		}
		owner->StartRightArmHitShake();
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

void BossEnemy::DamageShake() {

	// === 胴体の元の位置（シェイク前） ===
	Vector3 baseBodyPos = transform_.translate;

	// === シェイク後の胴体位置を計算 ===
	Vector3 bodyPos = baseBodyPos;

	// 落下シェイク（撃破演出）
	if (hp_ <= 0 && deathController_ && !deathController_->HasLanded()) {

		const Vector3 deathShake = deathController_->CalcShakeOffset();

		bodyPos.x += deathShake.x;
		bodyPos.y += deathShake.y;
		bodyPos.z += deathShake.z;
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
	// 色を反映
	// ==============================

	if (IsEnrageTransitioning()) {
		return;
	}

	Vector4 normalColor =
		isEnraged_
		? Vector4{ 1.0f, 0.1f, 0.1f, 1.0f }
	: Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };

	if (object3d_) {
		object3d_->SetColor(normalColor);
	}
}

void BossEnemy::StartRetreatAttack() {

	if (hp_ <= 0) return;
	retreatRequest_ = true;
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

	if (IsEnrageTransitioning()) {
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

void BossEnemy::StartCoreBreakReaction() {

	coreBreakReactionActive_ = true;
	coreBreakReactionTimer_ = 0.0f;
	coreBreakEffectPlayed_ = false;

	coreBreakKnockbackStart_ = transform_.translate;
	coreBreakKnockbackEnd_ = transform_.translate + Vector3{ 0.0f, 0.2f, 1.2f };

	// チャージ攻撃中断
	chargeActive_ = false;
	if (chargeBeam_) {
		chargeBeam_->Destroy();
	}
	if (chargeShot_.bullet && collisionManager_) {
		collisionManager_->Unregister(chargeShot_.bullet.get());
	}
	chargeShot_.bullet.reset();
	chargeShot_.obj.reset();

	// パーティクル
	auto* pm = KomEngine::System::GetParticleManager();
	if (pm) {
		const Vector3 pos = GetChargeCoreWorldPos();

		if (pm->Exists("charge_pulse")) {
			pm->Emit("charge_pulse", pos, 2);
		}
		if (pm->Exists("charge_core")) {
			pm->Emit("charge_core", pos, 36);
		}
		if (pm->Exists("moonLight")) {
			pm->Emit("moonLight", pos, 1);
		}
	}
}

void BossEnemy::UpdateCoreBreakReaction(float dt) {

	if (!coreBreakReactionActive_) {
		return;
	}

	coreBreakReactionTimer_ += dt;
	float t = std::clamp(coreBreakReactionTimer_ / coreBreakKnockbackTime_, 0.0f, 1.0f);

	float ease = 1.0f - (1.0f - t) * (1.0f - t);
	transform_.translate = MyMath::Lerp(coreBreakKnockbackStart_, coreBreakKnockbackEnd_, ease);

	if (t >= 1.0f) {
		coreBreakReactionActive_ = false;
	}
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

void BossEnemy::SetDizzyEffectActive(bool active) {

	if (dizzyStarController_) {
		dizzyStarController_->SetActive(active);
	}
}