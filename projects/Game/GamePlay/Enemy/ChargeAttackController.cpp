#include "ChargeAttackController.h"
#include "Game/Entity/Enemy/BossEnemy.h"
#include <utility>

class ChargeAttackController::IChargeState {
public:
	virtual ~IChargeState() = default;
	virtual void Update(ChargeAttackController& owner, float dt) = 0;
	virtual State GetType() const = 0;
};

class ChargeAttackController::ChargeStartState final : public IChargeState {
public:
	void Update(ChargeAttackController& owner, float dt) override {
		owner.t_ += dt;
		if (owner.t_ >= owner.telegraphTime_) {
			owner.BeginCharge();
		}
	}
	State GetType() const override { return State::ChargeStart; }
};

class ChargeAttackController::ChargingState final : public IChargeState {
public:
	void Update(ChargeAttackController& owner, float dt) override {
		owner.t_ += dt;
		if (owner.boss_->IsChargeCoreBroken()) {
			owner.InterruptCharge();
			return;
		}
		if (owner.t_ >= owner.chargeTime_) {
			owner.FireShot();
		}
	}
	State GetType() const override { return State::Charging; }
};

class ChargeAttackController::WaitShotEndState final : public IChargeState {
public:
	void Update(ChargeAttackController& owner, float dt) override {
		owner.t_ += dt;
		if (!owner.boss_->IsChargeBeamShotActive()) {
			owner.Finish();
		}
	}
	State GetType() const override { return State::WaitShotEnd; }
};

ChargeAttackController::ChargeAttackController() = default;
ChargeAttackController::~ChargeAttackController() = default;

void ChargeAttackController::Init() {

	active_ = false;
	state_.reset();
	t_ = 0.0f;
	targetLeft_ = false;
}

void ChargeAttackController::Start() {

	if (active_) return;
	if (!boss_) return;

	targetLeft_ = boss_->IsChargeTargetLeft();

	active_ = true;
	ChangeState(std::make_unique<ChargeStartState>());

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
	state_.reset();
	t_ = 0.0f;
}

void ChargeAttackController::Update(float dt) {

	if (!active_) return;
	if (!boss_) { ForceEnd(); return; }
	if (state_) {
		state_->Update(*this, dt);
	}
}

ChargeAttackController::State ChargeAttackController::GetState() const {
	return state_ ? state_->GetType() : State::None;
}

void ChargeAttackController::ChangeState(std::unique_ptr<IChargeState> nextState) {
	state_ = std::move(nextState);
	t_ = 0.0f;
}

void ChargeAttackController::BeginCharge() { 
	ChangeState(std::make_unique<ChargingState>());
}

void ChargeAttackController::InterruptCharge() {

	if (boss_) {
		boss_->SetChargeActive(false);
	}
	Finish();
}

void ChargeAttackController::FireShot() {

	if (boss_) {
		boss_->DeactivateChargeCore();
		boss_->StartChargeBeamShot(targetLeft_);
		boss_->SetChargeActive(false);
	}

	ChangeState(std::make_unique<WaitShotEndState>());
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
	state_.reset();
	t_ = 0.0f;
}