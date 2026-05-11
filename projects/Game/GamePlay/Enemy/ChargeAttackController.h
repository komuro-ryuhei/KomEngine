#pragma once
#include <cstdint>
#include <memory>

class Camera;
class Player;
class BossEnemy;

// 敵のチャージビームクラス
class ChargeAttackController {

public:

	ChargeAttackController();
	~ChargeAttackController();

	enum class State {
		None,
		ChargeStart,   // 予兆
		Charging,      // 破壊で中断できる
		Fire,          // 発射
		WaitShotEnd,   // 弾が消えるまで待つ
		End,
	};

public:

	void Init();

	void Update(float dt);

	void SetCamera(Camera* camera) { camera_ = camera; }
	void SetPlayer(Player* player) { player_ = player; }
	void SetBoss(BossEnemy* boss) { boss_ = boss; }

	void Start();
	void ForceEnd();

	bool IsActive() const { return active_; }
	State GetState() const;

	// 調整用
	void SetChargeTime(float t) { chargeTime_ = t; }
	void SetTelegraphTime(float t) { telegraphTime_ = t; }

private:

	class IChargeState;
	class ChargeStartState;
	class ChargingState;
	class WaitShotEndState;

	void ChangeState(std::unique_ptr<IChargeState> nextState);
	void BeginCharge();
	void InterruptCharge();
	void FireShot();
	void Finish();

private:
	Camera* camera_ = nullptr;
	Player* player_ = nullptr;
	BossEnemy* boss_ = nullptr;

	bool active_ = false;
	std::unique_ptr<IChargeState> state_;

	// どちらの腕をターゲットにするか
	bool targetLeft_ = false;

	// タイマー
	float t_ = 0.0f;

	// パラメータ
	float telegraphTime_ = 0.35f;
	float chargeTime_ = 3.0f;
};