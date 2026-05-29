// BossRushAttackController.cpp
#include "BossRushAttackController.h"

#include "Engine/Base/Camera/Camera.h"
#include "Game/Entity/Player/Player.h"
#include "Game/Entity/Enemy/BossEnemy.h"

#include <utility>
#include <fstream>
#include <iomanip>
#include <cmath>

class BossRushAttackController::IRushPhaseState {

public:

	virtual ~IRushPhaseState() = default;
	virtual void Update(BossRushAttackController& owner, float dt) = 0;
	virtual bool IsWarning() const { return false; }
};

class BossRushAttackController::WarningState final : public IRushPhaseState {

public:

	void Update(BossRushAttackController& owner, float dt) override { owner.UpdateWarning(dt); }
	bool IsWarning() const override { return true; }
};

class BossRushAttackController::ChargeState final : public IRushPhaseState {

public:

	void Update(BossRushAttackController& owner, float dt) override { owner.UpdateCharge(dt); }
};

class BossRushAttackController::RushState final : public IRushPhaseState {

public:

	void Update(BossRushAttackController& owner, float dt) override { owner.UpdateRush(dt); }
};

class BossRushAttackController::ReturnState final : public IRushPhaseState {

public:

	void Update(BossRushAttackController& owner, float dt) override { owner.UpdateReturn(dt); }
};

BossRushAttackController::BossRushAttackController() = default;
BossRushAttackController::~BossRushAttackController() = default;

void BossRushAttackController::Init() {

	state_.reset();
	warningTimer_ = 0.0f;
	warningVisible_ = true;

	warningSprite_ = std::make_unique<Sprite>();
	warningSprite_->Init("./Resources/images/exclamationMark.png", BlendType::BLEND_ALPHA);
	warningSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	warningSprite_->SetPosition(params_.warningPos);
	warningSprite_->SetSize(params_.warningSize);
	warningSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	warningSprite_->Update();

	rushAttack_.Init(boss_, player_);
}

void BossRushAttackController::Start() {

	if (!boss_ || !player_) {
		return;
	}

	boss_->CancelAllAttacks();

	warningTimer_ = 0.0f;
	warningVisible_ = true;

	ChangeState(std::make_unique<WarningState>());
}

void BossRushAttackController::Update(float dt) {

	if (!state_) {
		return;
	}

	if (!boss_ || !player_) {
		EndInternal();
		return;
	}

	state_->Update(*this, dt);
}

void BossRushAttackController::Draw() {

	if (IsWarningState() && warningSprite_ && warningVisible_) {
		warningSprite_->Draw();
	}
}

void BossRushAttackController::ForceEnd() {

	if (!state_) {
		return;
	}

	EndInternal();
}

void BossRushAttackController::LoadParamsFromJson(const std::string& path) {

	std::ifstream file(path);
	if (file.fail()) {
		return;
	}

	nlohmann::json j;
	file >> j;

	if (j.contains("rushAttack")) {
		params_.LoadJSON(j["rushAttack"]);
	}
}

void BossRushAttackController::SaveParamsToJson(const std::string& path) {

	nlohmann::json j;

	{
		std::ifstream ifs(path);
		if (!ifs.fail()) {
			try {
				ifs >> j;
			}
			catch (...) {
				j = nlohmann::json::object();
			}
		}
		else {
			j = nlohmann::json::object();
		}
	}

	nlohmann::json rushJson;
	params_.SaveJSON(rushJson);
	j["rushAttack"] = rushJson;

	std::ofstream ofs(path);
	ofs << std::setprecision(3) << j.dump(4);
}

void BossRushAttackController::UpdateWarning(float dt) {

	warningTimer_ += dt;

	const int blinkIndex = static_cast<int>(warningTimer_ / params_.warningBlinkInterval);
	warningVisible_ = ((blinkIndex % 2) == 0);

	if (warningSprite_) {
		warningSprite_->SetPosition(params_.warningPos);
		warningSprite_->SetSize(params_.warningSize);
		warningSprite_->Update();
	}

	if (warningTimer_ >= params_.warningTime) {
		warningVisible_ = false;
		rushAttack_.Start(params_);
		ChangeState(std::make_unique<ChargeState>());
	}
}

void BossRushAttackController::UpdateCharge(float dt) {

	if (rushAttack_.UpdateCharge(dt, params_)) {
		ChangeState(std::make_unique<RushState>());
	}
}

void BossRushAttackController::UpdateRush(float dt) {

	if (rushAttack_.UpdateRush(dt, params_)) {
		ChangeState(std::make_unique<ReturnState>());
	}
}

void BossRushAttackController::UpdateReturn(float dt) {

	if (rushAttack_.UpdateReturn(dt, params_)) {
		EndInternal();
	}
}

void BossRushAttackController::EndInternal() {

	rushAttack_.ForceEnd();

	state_.reset();
	warningTimer_ = 0.0f;
	warningVisible_ = false;
}

void BossRushAttackController::ChangeState(std::unique_ptr<IRushPhaseState> nextState) {
	state_ = std::move(nextState);
}

bool BossRushAttackController::IsWarningState() const {
	return state_ ? state_->IsWarning() : false;
}