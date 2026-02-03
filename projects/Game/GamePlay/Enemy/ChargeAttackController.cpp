#include "ChargeAttackController.h"
#include "Game/Entity/Enemy/BossEnemy.h"

void ChargeAttackController::Init() {

	active_ = false;
	state_ = State::None;
	t_ = 0.0f;
	targetLeft_ = false;
}

void ChargeAttackController::Start() {

	if (active_) return;
	if (!boss_) return;

	// Request側で決めたターゲット（左/右）
	bool wantLeft = boss_->IsChargeTargetLeft();

	// 既に壊れている腕を選んでいたら入れ替える
	const bool leftBroken = boss_->IsLeftArmBroken();
	const bool rightBroken = boss_->IsRightArmBroken();

	if (wantLeft && leftBroken && !rightBroken) {
		wantLeft = false;
	} else if (!wantLeft && rightBroken && !leftBroken) {
		wantLeft = true;
	}

	// 両腕とも壊れているならチャージ攻撃は成立しない
	if (leftBroken && rightBroken) {
		// マーカーを消す
		boss_->SetChargeActive(false);
		return;
	}

	targetLeft_ = wantLeft;

	active_ = true;
	state_ = State::ChargeStart;
	t_ = 0.0f;

	// マーカー表示用
	boss_->SetChargeTargetLeft(targetLeft_);
	boss_->SetChargeActive(true);
}

void ChargeAttackController::ForceEnd() {

	if (!active_) return;

	// マーカー消し
	if (boss_) {
		boss_->SetChargeActive(false);
	}

	active_ = false;
	state_ = State::None;
	t_ = 0.0f;
}

void ChargeAttackController::Update(float dt) {

	if (!active_) return;
	if (!boss_) { ForceEnd(); return; }

	t_ += dt;

	switch (state_) {

	case State::ChargeStart:
		if (t_ >= telegraphTime_) {
			BeginCharge();
		}
		break;

	case State::Charging:
	{
		// 部位破壊で中断
		const bool broken =
			(targetLeft_ && boss_->IsLeftArmBroken()) ||
			(!targetLeft_ && boss_->IsRightArmBroken());

		if (broken) {
			InterruptCharge();
			break;
		}

		if (t_ >= chargeTime_) {
			FireShot();
		}
	}
	break;

	case State::Fire:
		// 
		break;

	case State::WaitShotEnd:
		// 弾が消えたら終了
		if (!boss_->IsChargeBeamShotActive()) {
			Finish();
		}
		break;

	case State::End:
		Finish();
		break;

	default:
		break;
	}
}

void ChargeAttackController::BeginCharge() {

	state_ = State::Charging;
	t_ = 0.0f;
}

void ChargeAttackController::InterruptCharge() {

	// 
	if (boss_) {
		boss_->SetChargeActive(false);
	}
	state_ = State::End;
}

void ChargeAttackController::FireShot() {

	state_ = State::Fire;

	// 発射
	boss_->StartChargeBeamShot(targetLeft_);

	// 発射後は弾が消えるまで待つ
	state_ = State::WaitShotEnd;
	t_ = 0.0f;

	// 発射が終わったらマーカーは消す
	boss_->SetChargeActive(false);
}

void ChargeAttackController::Finish() {

	active_ = false;
	state_ = State::None;
	t_ = 0.0f;
}