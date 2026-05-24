#include "BossRushAttack.h"
#include "Game/Entity/Enemy/BossEnemy.h"
#include "Game/Entity/Player/Player.h"

#include <cmath>

void BossRushAttack::Init(BossEnemy* boss, Player* player) {

	boss_ = boss;
	player_ = player;
	active_ = false;
	timer_ = 0.0f;
}

void BossRushAttack::Start(const RushAttackParams& params) {

	if (!boss_ || !player_) {
		return;
	}

	active_ = true;
	timer_ = 0.0f;

	basePos_ = boss_->GetTranslate();
	const Vector3 playerPos = player_->GetTransform().translate;

	Vector3 toPlayer = MyMath::Subtract(playerPos, basePos_);
	toPlayer.y = 0.0f;

	if (MyMath::Length(toPlayer) <= 0.001f) {
		rushDir_ = { 0.0f, 0.0f, 1.0f };
	}
	else {
		rushDir_ = MyMath::Normalize(toPlayer);
	}

	// 溜め中に少し後ろへ引く位置
	backPos_ = MyMath::Subtract(basePos_, MyMath::Multiply(params.backAmount, rushDir_));

	// 突進先。プレイヤー位置に直接突っ込ませるより、方向固定 + 距離指定の方が暴れにくい。
	targetPos_ = MyMath::Add(basePos_, MyMath::Multiply(params.rushDistance, rushDir_));
}

bool BossRushAttack::UpdateCharge(float dt, const RushAttackParams& params) {

	if (!active_ || !boss_) {
		return true;
	}

	timer_ += dt;

	const float t = MyMath::Clamp01(timer_ / params.chargeMoveTime);
	const float ease = MyMath::EaseInOutCubic(t);

	Vector3 pos = MyMath::Lerp(basePos_, backPos_, ease);

	// 溜め中の震え。後ろへ引きながら少し振動させる。
	const float shake = std::sin(timer_ * params.chargeShakeSpeed) * params.chargeShakePower;
	pos.x += shake;

	boss_->SetTranslate(pos);

	if (timer_ >= params.chargeMoveTime) {
		timer_ = 0.0f;
		return true;
	}

	return false;
}

bool BossRushAttack::UpdateRush(float dt, const RushAttackParams& params) {

	if (!active_ || !boss_) {
		return true;
	}

	timer_ += dt;

	const float t = MyMath::Clamp01(timer_ / params.rushTime);
	const float ease = MyMath::EaseOutCubic(t);

	const Vector3 pos = MyMath::Lerp(backPos_, targetPos_, ease);
	boss_->SetTranslate(pos);

	if (timer_ >= params.rushTime) {
		timer_ = 0.0f;
		return true;
	}

	return false;
}

bool BossRushAttack::UpdateReturn(float dt, const RushAttackParams& params) {

	if (!active_ || !boss_) {
		return true;
	}

	timer_ += dt;

	const float t = MyMath::Clamp01(timer_ / params.returnTime);
	const float ease = MyMath::EaseInOutCubic(t);

	const Vector3 pos = MyMath::Lerp(targetPos_, basePos_, ease);
	boss_->SetTranslate(pos);

	if (timer_ >= params.returnTime) {
		boss_->SetTranslate(basePos_);
		active_ = false;
		timer_ = 0.0f;
		return true;
	}

	return false;
}

void BossRushAttack::ForceEnd() {

	if (boss_) {
		boss_->SetTranslate(basePos_);
	}

	active_ = false;
	timer_ = 0.0f;
}