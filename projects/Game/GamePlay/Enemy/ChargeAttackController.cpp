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

	targetLeft_ = boss_->IsChargeTargetLeft();

	active_ = true;
	state_ = State::ChargeStart;
	t_ = 0.0f;

	boss_->SetChargeTargetLeft(targetLeft_);
	boss_->SetChargeActive(true);
	boss_->ActivateChargeCore();
}

void ChargeAttackController::ForceEnd() {

	if (!active_) return;

	if (boss_) {
		boss_->SetChargeActive(false);
		boss_->DeactivateChargeCore();
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

		if (boss_->IsChargeCoreBroken()) {
			InterruptCharge();
			break;
		}

		if (t_ >= chargeTime_) {
			FireShot();
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

	if (boss_) {
		boss_->SetChargeActive(false);
		// boss_->DeactivateChargeCore();  // ←消す
	}
	state_ = State::End;
}

void ChargeAttackController::FireShot() {

	state_ = State::Fire;

	if (boss_) {
		boss_->DeactivateChargeCore();
		boss_->StartChargeBeamShot(targetLeft_);
		boss_->SetChargeActive(false);
	}

	state_ = State::WaitShotEnd;
	t_ = 0.0f;
}

void ChargeAttackController::Finish() {

	if (boss_) {
		// 崩壊中でなければ通常終了として消す
		if (!boss_->IsChargeCoreBroken()) {
			boss_->DeactivateChargeCore();
		}
		boss_->OnChargeAttackFinished();
	}

	active_ = false;
	state_ = State::None;
	t_ = 0.0f;
}