#pragma once
#include <memory>

class BossEnemy;
class BossRetreatAttack;
class BossMissileController;

class BossRetreatAttackController {

public:

	BossRetreatAttackController() = default;
	~BossRetreatAttackController() = default;

	void Init();

	void Update(float dt);

	void SetBoss(BossEnemy* boss) { boss_ = boss; }
	void SetRetreat(BossRetreatAttack* retreat) { retreat_ = retreat; }
	void SetMissileController(BossMissileController* missile) { missile_ = missile; }

	void Start();
	void ForceEnd();

	bool IsActive() const;
	BossRetreatAttack* GetRetreat() { return retreat_; }
	const BossRetreatAttack* GetRetreat() const { return retreat_; }

private:

	void ApplyToBoss();

private:

	BossEnemy* boss_ = nullptr;
	BossRetreatAttack* retreat_ = nullptr;
	BossMissileController* missile_ = nullptr;
};