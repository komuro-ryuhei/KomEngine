#pragma once

class BossEnemy;

/// ボスの片腕・両手攻撃の移動処理を管理する
class BossArmAttackController {

public:

	BossArmAttackController() = default;
	~BossArmAttackController() = default;

	/// 片腕攻撃更新
	void UpdateSingleArm(BossEnemy& boss, float dt);

	/// 両手攻撃更新
	void UpdateBothHands(BossEnemy& boss, float dt);
};