#pragma once
#include <memory>
#include <vector>
#include <random>
#include <algorithm>

#include "ChargeAttackController.h"

class Camera;
class Player;
class BossEnemy;
class BossMeteor;
class BossMeteorController;
class BossArmController;

class BossAttackManager {

public:

	enum class BossAttackBlock {
		ArmCombo,
		Charge,
		Meteor,
	};

	// 初期化用構造体
	struct InitDesc {
		Camera* camera = nullptr;
		Player* player = nullptr;
		BossEnemy* boss = nullptr;
		std::vector<std::unique_ptr<BossMeteor>>* meteors = nullptr;
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

public:

	// getter,setter
	BossMeteorController* GetMeteor() { return meteor_.get(); }
	BossArmController* GetArm() { return arm_.get(); }
	ChargeAttackController* GetCharge() { return charge_.get(); }

private:

	bool CanArmControlCamera(const UpdateFlags& flags) const;

private:

	InitDesc desc_{};

	std::unique_ptr<BossMeteorController> meteor_;
	std::unique_ptr<BossArmController> arm_;
	std::unique_ptr<ChargeAttackController> charge_;

	// 前フレームの腕攻撃アクティブ状態
	bool prevArmActive_ = false;

	// 攻撃ブロック管理
	std::vector<BossAttackBlock> blockQueue_;
	size_t blockIndex_ = 0;
	BossAttackBlock currentBlock_ = BossAttackBlock::ArmCombo;

	std::mt19937 rng_{};
	bool queueInited_ = false;

	// 次のブロック順を作る（ここでランダム化）
	void RebuildBlockQueue();

	// 現在ブロックがアクティブか
	bool IsBlockActive(BossAttackBlock b) const;

	// ブロック開始
	void StartBlock(BossAttackBlock b);

};