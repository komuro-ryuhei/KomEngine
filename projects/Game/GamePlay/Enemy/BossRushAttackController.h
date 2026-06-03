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

	// アクティブかどうか
	bool IsActive() const { return state_ != nullptr; }

	// スタン状態かどうか
	bool IsStunned() const { return isStunned_; }

	bool IsRushSlowEffectActive() const { return rushSlowEffectActive_; }
	float GetRushSlowEffectIntensity() const { return rushSlowEffectIntensity_; }

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
	class KnockbackState;
	class StunState;
	class ReturnState;

	void ChangeState(std::unique_ptr<IRushPhaseState> nextState);
	bool IsWarningState() const;

	void UpdateWarning(float dt);
	void UpdateCharge(float dt);
	void UpdateRush(float dt);
	void UpdateKnockback(float dt);
	void UpdateStun(float dt);
	void UpdateReturn(float dt);

	bool ShouldStartRushSlowMotion() const;
	void ResetRushSlowEffect();

	bool CheckBarrierGuard() const;
	void StartKnockback();
	void StartStun();

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

	// ----------------------- バリア成功時の弾かれ・スタン ----------------------- //
	bool isStunned_ = false;

	float knockbackTimer_ = 0.0f;
	float knockbackTime_ = 1.2f;

	float stunTimer_ = 0.0f;
	float stunTime_ = 5.0f;

	// バリア判定を少し甘くする追加半径
	float barrierGuardExtraRadius_ = 1.5f;

	Vector3 knockbackStartPos_{};
	Vector3 knockbackEndPos_{};

	// ノックバック演出用
	Vector3 knockbackDir_{};

	// ノックバックで少し行き過ぎる量
	float knockbackOvershootAmount_ = 1.0f;

	// 2〜3回跳ねるための設定
	int knockbackBounceCount_ = 3;
	float knockbackHopHeight_ = 1.2f;
	float knockbackBounceDamping_ = 0.55f;

	float knockbackShakePower_ = 0.08f;

	// スタン演出用
	Vector3 stunBasePos_{};
	Vector3 stunBaseRotate_{};

	float stunShakePower_ = 0.08f;
	float stunShakeSpeed_ = 55.0f;
	float stunRotateSpeed_ = 1.6f;
	float stunRotateAmount_ = 0.45f;

	// ----------------------- 突進直前スロー演出 ----------------------- //
	bool rushSlowTriggered_ = false;
	bool rushSlowEffectActive_ = false;

	float rushSlowEffectTimer_ = 0.0f;
	float rushSlowEffectDuration_ = 1.2f;

	// 敵だけ遅くする倍率
	float rushSlowScale_ = 0.012f;

	// この距離以内に入ったらスロー開始
	float rushSlowStartDistance_ = 12.0f;

	// ポストエフェクト用
	float rushSlowEffectIntensity_ = 0.0f;
	// ---------------------------------------------------------------- //
};