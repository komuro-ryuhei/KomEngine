// BossRushAttack.h
#pragma once

#include "Engine/lib/Math/MyMath.h"
#include "BossRushAttackParams.h"

class BossEnemy;
class Player;

// ボス突進攻撃
class BossRushAttack {
public:
	BossRushAttack() = default;
	~BossRushAttack() = default;

	void Init(BossEnemy* boss, Player* player);

	// 突進開始位置・方向などを決める
	void Start(const RushAttackParams& params);

	// 各フェーズの移動更新。
	// true を返したら、そのフェーズの移動が完了。
	bool UpdateCharge(float dt, const RushAttackParams& params);
	bool UpdateRush(float dt, const RushAttackParams& params);
	bool UpdateReturn(float dt, const RushAttackParams& params);

	void ForceEnd();

	bool IsActive() const { return active_; }

	const Vector3& GetBasePos() const { return basePos_; }
	const Vector3& GetBackPos() const { return backPos_; }
	const Vector3& GetTargetPos() const { return targetPos_; }
	const Vector3& GetRushDir() const { return rushDir_; }

private:

	BossEnemy* boss_ = nullptr;
	Player* player_ = nullptr;

	bool active_ = false;

	float timer_ = 0.0f;

	Vector3 basePos_{};
	Vector3 backPos_{};
	Vector3 targetPos_{};
	Vector3 rushDir_{};
};