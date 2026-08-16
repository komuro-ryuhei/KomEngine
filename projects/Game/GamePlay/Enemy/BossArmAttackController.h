#pragma once

#include "Engine/lib/Vector/Vector.h"

class BossEnemy;

// ボスの片腕・両手攻撃の管理
class BossArmAttackController {

public:

	BossArmAttackController() = default;
	~BossArmAttackController() = default;

	// 片腕攻撃の予備動作を開始
	void BeginSingleArmTelegraph();

	// 両手攻撃の予備動作を開始
	void BeginBothHandsTelegraph();

	// 腕コンボ終了時の内部状態を初期化
	void FinishComboState();

	// 腕攻撃を完全にリセット
	void Reset();

	// 怒り状態に応じて腕の戻り速度を変更
	void SetEnraged(
		bool enraged,
		float speedMultiplier
	);

	// 現在、片腕を伸ばしている途中か
	bool IsExtending() const {
		return isExtending_;
	}

	// 片腕攻撃更新
	void UpdateSingleArm(
		BossEnemy& boss,
		float dt
	);

	/// 両手攻撃更新
	void UpdateBothHands(
		BossEnemy& boss,
		float dt
	);

private:

	// =====================================================
	// 片腕攻撃
	// =====================================================

	// 現在腕を伸ばしているか
	bool isExtending_ = true;

	// 予備動作中か
	bool armTelegraphActive_ = false;

	// 予備動作経過時間
	float armTelegraphTimer_ = 0.0f;

	// 後ろに腕を引く時間
	float armTelegraphBackTime_ = 0.20f;

	// 予備動作全体の時間
	float armTelegraphDuration_ = 1.2f;

	// 腕を後ろに引く距離
	float armTelegraphBackAmount_ = 1.8f;

	// 震え幅
	float armTelegraphShakeAmount_ = 0.050f;

	// 震え速度
	float armTelegraphShakeFreq_ = 65.0f;

	// 腕を伸ばす速度
	float armRushSpeed_ = 0.15f;

	// 予備動作開始位置
	Vector3 armTelegraphStartPos_{};

	// 予備動作で引いた先
	Vector3 armTelegraphTargetPos_{};

	// =====================================================
	// 両手攻撃
	// =====================================================

	bool bothTelegraphActive_ = false;
	float bothTelegraphTimer_ = 0.0f;

	float bothTelegraphBackTime_ = 0.20f;
	float bothTelegraphDuration_ = 1.4f;

	float bothTelegraphBackAmount_ = 2.0f;
	float bothTelegraphShakeAmount_ = 0.06f;
	float bothTelegraphShakeFreq_ = 60.0f;

	float bothRushSpeed_ = 0.16f;

	Vector3 leftBothTelegraphStartPos_{};
	Vector3 rightBothTelegraphStartPos_{};

	Vector3 leftBothTelegraphTargetPos_{};
	Vector3 rightBothTelegraphTargetPos_{};

	// 左右それぞれ戻り中かどうかを管理
	bool leftExtending_ = true;
	bool rightExtending_ = true;

	// =====================================================
	// 腕攻撃共通
	// =====================================================

	// 風切りエフェクト
	float armWindSlashFxTimer_ = 0.0f;
	float armWindSlashFxInterval_ = 0.03f;

	// 腕の戻り速度
	float armReturnSpeedSingle_ = 0.5f;
	float armReturnSpeedBoth_ = 0.6f;

	// 怒り状態から通常に戻すための基準値
	float baseArmReturnSpeedSingle_ = 0.5f;
	float baseArmReturnSpeedBoth_ = 0.6f;
};