// BossRushAttackController.cpp
#include "BossRushAttackController.h"

#include "Engine/Base/Camera/Camera.h"
#include "Game/Entity/Player/Player.h"
#include "Game/Entity/Enemy/BossEnemy.h"

#include <utility>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

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

class BossRushAttackController::KnockbackState final : public IRushPhaseState {

public:

	void Update(BossRushAttackController& owner, float dt) override {
		owner.UpdateKnockback(dt);
	}
};

class BossRushAttackController::StunState final : public IRushPhaseState {

public:

	void Update(BossRushAttackController& owner, float dt) override {
		owner.UpdateStun(dt);
	}
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

	isStunned_ = false;
	knockbackTimer_ = 0.0f;
	stunTimer_ = 0.0f;
	knockbackStartPos_ = {};
	knockbackEndPos_ = {};
	knockbackDir_ = {};
	stunBasePos_ = {};
	stunBaseRotate_ = {};

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

	isStunned_ = false;
	knockbackTimer_ = 0.0f;
	stunTimer_ = 0.0f;
	knockbackStartPos_ = {};
	knockbackEndPos_ = {};
	knockbackDir_ = {};
	stunBasePos_ = {};
	stunBaseRotate_ = {};

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
			} catch (...) {
				j = nlohmann::json::object();
			}
		} else {
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

	// 先に突進移動を進める
	const bool rushFinished = rushAttack_.UpdateRush(dt, params_);

	// 突進中にプレイヤーがバリアを張っていて、距離が近ければ防御成功
	if (CheckBarrierGuard()) {
		StartKnockback();
		return;
	}

	if (rushFinished) {
		ChangeState(std::make_unique<ReturnState>());
	}
}

void BossRushAttackController::UpdateKnockback(float dt) {

	if (!boss_) {
		EndInternal();
		return;
	}

	knockbackTimer_ += dt;

	float t = 0.0f;
	if (knockbackTime_ > 0.0f) {
		t = knockbackTimer_ / knockbackTime_;
	}
	t = MyMath::Clamp01(t);

	// --------------------------------------------------
	// 横移動：
	// 最初に強く吹っ飛び、最後は少し減速して元位置へ近づく
	// --------------------------------------------------
	const float moveEase = MyMath::EaseOutCubic(t);

	Vector3 pos = MyMath::Lerp(knockbackStartPos_, knockbackEndPos_, moveEase);

	// --------------------------------------------------
	// にゃんこ大戦争っぽい複数バウンド
	// tを bounceCount 回分に分割して、
	// 各区間ごとに 0→山→0 のジャンプを作る
	// --------------------------------------------------
	const int bounceCount = std::max(1, knockbackBounceCount_);

	float bouncePhase = t * static_cast<float>(bounceCount);
	int bounceIndex = static_cast<int>(bouncePhase);

	if (bounceIndex >= bounceCount) {
		bounceIndex = bounceCount - 1;
	}

	float localT = bouncePhase - static_cast<float>(bounceIndex);
	localT = MyMath::Clamp01(localT);

	// 1回目は大きく、2回目以降は小さくする
	float damping = 1.0f;
	for (int i = 0; i < bounceIndex; ++i) {
		damping *= knockbackBounceDamping_;
	}

	// 各バウンドで 0 → 1 → 0
	const float hop = std::sin(localT * 3.14159265f) * knockbackHopHeight_ * damping;
	pos.y += hop;

	// --------------------------------------------------
	// 着地っぽさ：
	// 各バウンドの終わり付近で少し横揺れを入れる
	// --------------------------------------------------
	const float globalDamping = 1.0f - t;
	const float shake = globalDamping * knockbackShakePower_;

	pos.x += std::sin(knockbackTimer_ * 90.0f) * shake;
	pos.z += std::cos(knockbackTimer_ * 83.0f) * shake;

	boss_->SetTranslate(pos);

	if (knockbackTimer_ >= knockbackTime_) {
		boss_->SetTranslate(knockbackEndPos_);
		StartStun();
	}
}

void BossRushAttackController::UpdateStun(float dt) {

	stunTimer_ += dt;

	float t = 0.0f;
	if (stunTime_ > 0.0f) {
		t = stunTimer_ / stunTime_;
	}
	t = MyMath::Clamp01(t);

	// スタン終盤に向かって揺れ・回転を弱くする
	// 1.0 → 0.0 にだんだん減る
	const float decay = 1.0f - MyMath::EaseInOutCubic(t);

	if (boss_) {

		// -----------------------------
		// 位置：元位置を中心に細かく震える
		// 時間経過でだんだん震えを小さくする
		// -----------------------------
		Vector3 pos = stunBasePos_;

		pos.x += std::sin(stunTimer_ * stunShakeSpeed_) * stunShakePower_ * decay;
		pos.y += std::cos(stunTimer_ * stunShakeSpeed_ * 0.83f) * stunShakePower_ * 0.45f * decay;
		pos.z += std::sin(stunTimer_ * stunShakeSpeed_ * 1.17f) * stunShakePower_ * decay;

		boss_->SetTranslate(pos);

		// -----------------------------
		// 回転：だんだん小さくなる
		// 終了時には stunBaseRotate_ に自然に近づく
		// -----------------------------
		Vector3 rot = stunBaseRotate_;

		rot.y += std::sin(stunTimer_ * stunRotateSpeed_) * stunRotateAmount_ * decay;
		rot.z += std::sin(stunTimer_ * stunRotateSpeed_ * 1.7f) * 0.08f * decay;

		boss_->SetRotate(rot);
	}

	if (stunTimer_ >= stunTime_) {

		isStunned_ = false;

		if (boss_) {
			boss_->SetTranslate(stunBasePos_);
			boss_->SetRotate(stunBaseRotate_);

			// スタン星演出終了
			boss_->SetDizzyEffectActive(false);
		}

		EndInternal();
	}
}

bool BossRushAttackController::CheckBarrierGuard() const {

	if (!boss_ || !player_) {
		return false;
	}

	if (!player_->IsBarrierActive()) {
		return false;
	}

	const Vector3 bossPos = boss_->GetTranslate();
	const Vector3 barrierPos = player_->GetBarrierPosition();

	Vector3 diff = MyMath::Subtract(bossPos, barrierPos);

	// 高さ差で失敗しにくいように、XZ平面だけで見る
	diff.y = 0.0f;

	const float distance = MyMath::Length(diff);

	const float bossRadius = boss_->GetCollisionRadius();
	const float barrierRadius = player_->GetBarrierRadius();

	const float hitRange = bossRadius + barrierRadius + barrierGuardExtraRadius_;

	return distance <= hitRange;
}

void BossRushAttackController::StartKnockback() {

	if (!boss_) {
		EndInternal();
		return;
	}

	knockbackTimer_ = 0.0f;

	knockbackStartPos_ = boss_->GetTranslate();
	knockbackEndPos_ = rushAttack_.GetBasePos();

	// 今の位置から元位置へ向かう方向
	knockbackDir_ = MyMath::Subtract(knockbackEndPos_, knockbackStartPos_);
	knockbackDir_.y = 0.0f;

	if (MyMath::Length(knockbackDir_) <= 0.001f) {
		knockbackDir_ = MyMath::Multiply(-1.0f, rushAttack_.GetRushDir());
	} else {
		knockbackDir_ = MyMath::Normalize(knockbackDir_);
	}

	warningVisible_ = false;

	ChangeState(std::make_unique<KnockbackState>());
}

void BossRushAttackController::StartStun() {

	isStunned_ = true;
	stunTimer_ = 0.0f;

	if (boss_) {
		stunBasePos_ = rushAttack_.GetBasePos();
		stunBaseRotate_ = boss_->GetRotate();

		boss_->SetTranslate(stunBasePos_);

		// スタン星演出開始
		boss_->SetDizzyEffectActive(true);
	}

	ChangeState(std::make_unique<StunState>());
}

void BossRushAttackController::UpdateReturn(float dt) {

	if (rushAttack_.UpdateReturn(dt, params_)) {
		EndInternal();
	}
}

void BossRushAttackController::EndInternal() {

	if (boss_) {
		boss_->SetDizzyEffectActive(false);
	}

	if (isStunned_ && boss_) {
		boss_->SetTranslate(stunBasePos_);
		boss_->SetRotate(stunBaseRotate_);
	}

	rushAttack_.ForceEnd();

	state_.reset();
	warningTimer_ = 0.0f;
	warningVisible_ = false;

	isStunned_ = false;
	knockbackTimer_ = 0.0f;
	stunTimer_ = 0.0f;

	knockbackStartPos_ = {};
	knockbackEndPos_ = {};
	knockbackDir_ = {};

	stunBasePos_ = {};
	stunBaseRotate_ = {};
}

void BossRushAttackController::ChangeState(std::unique_ptr<IRushPhaseState> nextState) {
	state_ = std::move(nextState);
}

bool BossRushAttackController::IsWarningState() const {
	return state_ ? state_->IsWarning() : false;
}