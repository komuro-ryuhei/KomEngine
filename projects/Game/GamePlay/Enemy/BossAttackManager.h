#pragma once
#include <memory>
#include <vector>
#include <random>
#include <algorithm>

#include "ChargeAttackController.h"
#include "BossMissileController.h"
#include "BossRetreatAttackController.h"
#include "BossRushAttackController.h"
#include "Game/Entity/Enemy/BossRetreatAttack.h"

class Camera;
class Player;
class BossEnemy;
class BossMeteor;
class BossMeteorController;
class BossArmController;
class BossMissile;
class BossMissileController;
class BossRetreatAttack;
class BossRetreatAttackController;
class BossRushAttackController;

class BossAttackManager {

public:

	enum class BossAttackBlock {
		ArmCombo,
		Charge,
		Meteor,
		Rush,
	};

	// 初期化用構造体
	struct InitDesc {
		Camera* camera = nullptr;
		Player* player = nullptr;
		BossEnemy* boss = nullptr;
		std::vector<std::unique_ptr<BossMeteor>>* meteors = nullptr;
		std::vector<std::unique_ptr<BossMissile>>* missiles = nullptr;
	};

	// BossTestScene側で毎フレーム渡せるフラグ
	struct UpdateFlags {
		bool koActive = false;
		bool isMainPhase = true;
		bool swordCamActive = false;
		bool isCameraFollowPlayer = true;
	};

public:

	BossAttackManager() = default;
	~BossAttackManager();

	void Init(const InitDesc& desc);

	// 毎フレーム呼ぶ
	void Update(float dt, const UpdateFlags& flags);

	// 現在の攻撃が終了したときに呼ぶ
	void OnCurrentAttackFinished();

	// メテオ操作（デバッグ・演出開始用）
	void StartMeteor();
	void ForceEndMeteor();

	bool IsMeteorActive() const;
	bool IsChargeActive() const;
	bool IsAnyAttackActive() const;

	// 
	void StartEnragePause(float duration);
	bool IsEnragePaused() const { return enragePauseActive_; }

	// ---------------- デバッグ用追加 ---------------- //
	void SetDebugPauseAllAttacks(bool pause);
	bool IsDebugPauseAllAttacks() const { return debugPauseAllAttacks_; }

	// 全攻撃を一回止めて、チャージだけ出したいときに使う
	void RequestDebugChargeAttack(bool targetLeft);

	// ---------------- デバッグ固定攻撃モード ----------------
	void SetDebugFixedAttackMode(bool enable);
	bool IsDebugFixedAttackMode() const { return debugFixedAttackMode_; }

	void SetDebugFixedAttackBlock(BossAttackBlock block);
	BossAttackBlock GetDebugFixedAttackBlock() const { return debugFixedAttackBlock_; }

	const char* GetAttackBlockName(BossAttackBlock block) const;
	// ----------------------------------------------- //

public:

	// getter,setter
	BossMeteorController* GetMeteor() { return meteor_.get(); }
	BossArmController* GetArm() { return arm_.get(); }
	ChargeAttackController* GetCharge() { return charge_.get(); }
	BossMissileController* GetMissile() { return missile_.get(); }
	BossRetreatAttackController* GetRetreat() { return retreat_.get(); }
	BossRushAttackController* GetRush() { return rush_.get(); }

private:

	bool CanArmControlCamera(const UpdateFlags& flags) const;

private:

	InitDesc desc_{};

	std::unique_ptr<BossMeteorController> meteor_;
	std::unique_ptr<BossArmController> arm_;
	std::unique_ptr<ChargeAttackController> charge_;
	std::unique_ptr<BossMissileController> missile_;
	std::unique_ptr<BossRetreatAttack> retreatAttack_;
	std::unique_ptr<BossRetreatAttackController> retreat_;
	std::unique_ptr<BossRushAttackController> rush_;

	// 前フレームの腕攻撃アクティブ状態
	bool prevArmActive_ = false;

	// 攻撃ブロック管理
	std::vector<BossAttackBlock> blockQueue_;
	size_t blockIndex_ = 0;
	BossAttackBlock currentBlock_ = BossAttackBlock::ArmCombo;

	std::mt19937 rng_{};
	bool queueInited_ = false;

	// ブロック開始フラグ
	bool blockStarted_ = false;

	// 攻撃ブロック間の待機
	bool waitingNextBlock_ = false;
	float nextBlockWaitTimer_ = 0.0f;
	float nextBlockWaitDuration_ = 1.5f;        // 通常時の攻撃間隔
	float enragedNextBlockWaitDuration_ = 1.0f; // 怒り時の攻撃間隔

	// 怒り状態の一時停止
	bool enragePauseActive_ = false;
	// 一時停止タイマー
	float enragePauseTimer_ = 0.0f;
	// 一時停止の継続時間
	float enragePauseDuration_ = 0.0f;

	// ---------------- デバッグ用追加 ----------------
	bool debugPauseAllAttacks_ = false;


	// ---------------- デバッグ固定攻撃モード ----------------
	bool debugFixedAttackMode_ = false;
	BossAttackBlock debugFixedAttackBlock_ = BossAttackBlock::Rush;

	bool debugFixedBlockStarted_ = false;
	bool debugFixedWaiting_ = false;
	float debugFixedWaitTimer_ = 0.0f;
	float debugFixedWaitDuration_ = 1.0f;

	// -----------------------------------------------

private:

	// 次のブロック順を作る
	void RebuildBlockQueue();

	// 現在ブロックがアクティブか
	bool IsBlockActive(BossAttackBlock b) const;

	// ブロック開始
	bool StartBlock(BossAttackBlock b);

	// すべての攻撃を止める
	void StopAllAttacks(float dt);

	// デバッグ固定攻撃モードの更新
	void UpdateDebugFixedAttack(float dt, const UpdateFlags& flags);
	bool IsCurrentBlockFinished(BossAttackBlock block) const;
	void ForceEndAllAttacks();
};