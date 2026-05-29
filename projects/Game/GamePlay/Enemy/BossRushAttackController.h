#pragma once
#include <memory>
#include <string>

#include "Engine/Base/2d/Sprite/Sprite.h"
#include "Game/Entity/Enemy/BossRushAttack.h"
#include "Game/Entity/Enemy/BossRushAttackParams.h"

class Camera;
class Player;
class BossEnemy;

class BossRushAttackController {

public:

	BossRushAttackController();
	~BossRushAttackController();

	void Init();
	void Update(float dt);
	void Draw();

	void Start();
	void ForceEnd();

	bool IsActive() const { return state_ != nullptr; }

public:

	RushAttackParams& GetParams() { return params_; }
	const RushAttackParams& GetParams() const { return params_; }

	void SetCamera(Camera* cam) { camera_ = cam; }
	void SetPlayer(Player* player) { player_ = player; rushAttack_.Init(boss_, player_); }
	void SetBoss(BossEnemy* boss) { boss_ = boss; rushAttack_.Init(boss_, player_); }

	void LoadParamsFromJson(const std::string& path);
	void SaveParamsToJson(const std::string& path);

private:

	class IRushPhaseState;
	class WarningState;
	class ChargeState;
	class RushState;
	class ReturnState;

	void ChangeState(std::unique_ptr<IRushPhaseState> nextState);
	bool IsWarningState() const;

	void UpdateWarning(float dt);
	void UpdateCharge(float dt);
	void UpdateRush(float dt);
	void UpdateReturn(float dt);

	void EndInternal();

private:

	Camera* camera_ = nullptr;
	Player* player_ = nullptr;
	BossEnemy* boss_ = nullptr;

	BossRushAttack rushAttack_;
	std::unique_ptr<IRushPhaseState> state_;

	// 警告表示
	std::unique_ptr<Sprite> warningSprite_ = nullptr;
	float warningTimer_ = 0.0f;
	bool warningVisible_ = true;

	RushAttackParams params_;
};