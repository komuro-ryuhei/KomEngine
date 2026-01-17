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

	// Boss側に「どっちを狙わせたいか」は RequestChargeAttack で入っている想定
	targetLeft_ = boss_->IsChargeTargetLeft();

	active_ = true;
	state_ = State::ChargeStart;
	t_ = 0.0f;

	// マーカー表示用
	boss_->SetChargeActive(true);
	boss_->SetChargeTargetLeft(targetLeft_);
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
		// FireShot() 内で遷移させる
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
	// 失敗じゃなく「プレイヤー成功」なので発射しない
	if (boss_) {
		boss_->SetChargeActive(false);
	}
	state_ = State::End;
}

void ChargeAttackController::FireShot() {
	state_ = State::Fire;

	// 発射（Boss側で弾生成）
	boss_->StartChargeBeamShot(targetLeft_);

	// 発射後は弾が消えるまで待つ（当たり or 寿命）
	state_ = State::WaitShotEnd;
	t_ = 0.0f;

	// 発射が終わったらマーカーは消してOK（狙わせフェーズは終わり）
	boss_->SetChargeActive(false);
}

void ChargeAttackController::Finish() {
	active_ = false;
	state_ = State::None;
	t_ = 0.0f;
}
\