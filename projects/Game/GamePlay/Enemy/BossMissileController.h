#pragma once
#include <memory>
#include <vector>
#include <array>

#include "Engine/lib/Math/MyMath.h"

class Camera;
class Player;
class BossEnemy;
class BossMissile;

class BossMissileController {

public:

	struct Params {
		float telegraphTime = 1.0f;
		float radius = 2.5f;
		float height = 2.0f;
		float speed = 0.2f;
		float launchTimeout = 6.0f;

		float curveSideOffset = 6.0f; // 外側へのふくらみ
		float curveUpOffset = 1.5f;   // 少し上にも持ち上げる
		float curveDuration = 6.0f;   // 着弾までの時間

	};

	BossMissileController() = default;
	~BossMissileController() = default;

	void Init();

	void Update(float dt);

	void SetCamera(Camera* camera) { camera_ = camera; }
	void SetPlayer(Player* player) { player_ = player; }
	void SetBoss(BossEnemy* boss) { boss_ = boss; }
	void SetMissiles(std::vector<std::unique_ptr<BossMissile>>* missiles) { missiles_ = missiles; }

	void Start();
	void ForceEnd();

	bool IsActive() const { return phase_ != Phase::None; }
	bool DidHitPlayer() const { return hitPlayer_; }

	Params& GetParams() { return params_; }
	const Params& GetParams() const { return params_; }

private:

	enum class Phase {
		None,
		Telegraph,
		Launch,
	};

	void UpdateTelegraph(float dt);
	void UpdateLaunch(float dt);

private:

	Camera* camera_ = nullptr;
	Player* player_ = nullptr;
	BossEnemy* boss_ = nullptr;
	std::vector<std::unique_ptr<BossMissile>>* missiles_ = nullptr;

	Phase phase_ = Phase::None;
	float timer_ = 0.0f;
	bool hitPlayer_ = false;

	Params params_;
};