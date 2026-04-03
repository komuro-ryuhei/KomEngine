#include "BossAttackManager.h"
#include "BossMeteorController.h"
#include "BossArmController.h"
#include "BossMissileController.h"
#include "BossRetreatAttackController.h"

#include "Game/Entity/Enemy/BossEnemy.h"
#include "Game/Entity/Enemy/BossRetreatAttack.h"

#include <cassert>

BossAttackManager::~BossAttackManager() = default;

void BossAttackManager::Init(const InitDesc& desc) {

	desc_ = desc;

	meteor_ = std::make_unique<BossMeteorController>();
	meteor_->Init();
	meteor_->SetCamera(desc_.camera);
	meteor_->SetPlayer(desc_.player);
	meteor_->SetBoss(desc_.boss);
	meteor_->SetMeteors(desc_.meteors);

	arm_ = std::make_unique<BossArmController>();
	arm_->Init(desc_.camera, desc_.boss);

	charge_ = std::make_unique<ChargeAttackController>();
	charge_->Init();
	charge_->SetCamera(desc_.camera);
	charge_->SetPlayer(desc_.player);
	charge_->SetBoss(desc_.boss);

	missile_ = std::make_unique<BossMissileController>();
	missile_->Init();
	missile_->SetCamera(desc_.camera);
	missile_->SetPlayer(desc_.player);
	missile_->SetBoss(desc_.boss);
	missile_->SetMissiles(desc_.missiles);

	retreatAttack_ = std::make_unique<BossRetreatAttack>();
	retreatAttack_->Init();

	retreat_ = std::make_unique<BossRetreatAttackController>();
	retreat_->Init();
	retreat_->SetBoss(desc_.boss);
	retreat_->SetRetreat(retreatAttack_.get());
	retreat_->SetMissileController(missile_.get());

	// 乱数初期化
	std::random_device rd;
	rng_ = std::mt19937(rd());
	queueInited_ = false;

	waitingNextBlock_ = false;
	nextBlockWaitTimer_ = 0.0f;
}

bool BossAttackManager::CanArmControlCamera(const UpdateFlags& flags) const {

	return
		!flags.koActive
		&& flags.isMainPhase
		&& !waitingNextBlock_
		&& !IsMeteorActive()
		&& !IsChargeActive()
		&& !flags.swordCamActive
		&& flags.isCameraFollowPlayer;
}

void BossAttackManager::Update(float dt, const UpdateFlags& flags) {

	StopAllAttacks(dt);

	// 怒り遷移中の一時停止
	if (enragePauseActive_) {
		enragePauseTimer_ += dt;

		if (retreat_ && retreat_->IsActive()) {
			retreat_->ForceEnd();
		}
		if (missile_ && missile_->IsActive()) {
			missile_->ForceEnd();
		}

		// 腕コントローラだけ見た目更新が必要なら残す
			// 腕更新
		if (arm_) {
			if (waitingNextBlock_) {
				arm_->Update(dt, false);
			} else {
				arm_->Update(dt, CanArmControlCamera(flags));
			}
		}

		if (enragePauseTimer_ >= enragePauseDuration_) {
			enragePauseActive_ = false;
			enragePauseTimer_ = 0.0f;
		}

		return;
	}

	// 攻撃間の待機
	if (waitingNextBlock_) {
		nextBlockWaitTimer_ += dt;

		if (nextBlockWaitTimer_ >= nextBlockWaitDuration_) {
			waitingNextBlock_ = false;
			nextBlockWaitTimer_ = 0.0f;
		}
	}

	const bool wasMeteorActive = (meteor_ && meteor_->IsActive());
	const bool wasChargeActive = (charge_ && charge_->IsActive());

	// 腕更新
	if (arm_) {
		arm_->Update(dt, CanArmControlCamera(flags));
	}

	// メテオ中にKO/フェーズ外なら強制終了
	if (meteor_ && meteor_->IsActive()) {
		if (flags.koActive || !flags.isMainPhase) {
			meteor_->ForceEnd();
		}
	}

	const bool armComboActive = (desc_.boss && desc_.boss->IsArmComboActive());

	// ===== メテオ開始条件 =====
	const bool canStartMeteor =
		!flags.koActive &&
		flags.isMainPhase &&
		!waitingNextBlock_ &&
		!(meteor_ && meteor_->IsActive()) &&
		!(retreat_ && retreat_->IsActive()) &&
		!(charge_ && charge_->IsActive()) &&
		!armComboActive;

	if (canStartMeteor && desc_.boss && desc_.boss->ConsumeMeteorRequest()) {
		meteor_->Start();
	}

	// ===== チャージビーム開始条件 =====
	const bool canStartCharge =
		!flags.koActive &&
		flags.isMainPhase &&
		!waitingNextBlock_ &&
		!(charge_ && charge_->IsActive()) &&
		!(meteor_ && meteor_->IsActive()) &&
		!(retreat_ && retreat_->IsActive()) &&
		!armComboActive;

	if (canStartCharge && desc_.boss && desc_.boss->ConsumeChargeRequest()) {
		charge_->Start();
	}

	// 離脱開始
	const bool canStartRetreat =
		!flags.koActive &&
		flags.isMainPhase &&
		!waitingNextBlock_ &&
		!(meteor_ && meteor_->IsActive()) &&
		!(charge_ && charge_->IsActive()) &&
		!(retreat_ && retreat_->IsActive());

	if (canStartRetreat && desc_.boss && desc_.boss->ConsumeRetreatRequest()) {
		retreat_->Start();
	}

	// 離脱更新
	if (retreat_ && retreat_->IsActive()) {
		retreat_->Update(dt);
	}

	// ミサイル更新
	if (missile_ && missile_->IsActive()) {
		missile_->Update(dt);
	}

	// メテオ更新
	if (meteor_ && meteor_->IsActive()) {
		meteor_->Update(dt);
	}

	// チャージ更新
	if (charge_ && charge_->IsActive()) {
		charge_->Update(dt);
	}

	// チャージが終わった瞬間にBossへ通知
	const bool isChargeActive = (charge_ && charge_->IsActive());
	if (wasChargeActive && !isChargeActive && desc_.boss) {
		desc_.boss->OnChargeAttackFinished();
	}

	// ================== ブロック順制御（ランダム） ================== //
	if (!queueInited_) {
		RebuildBlockQueue();

		waitingNextBlock_ = true;
		nextBlockWaitTimer_ = 0.0f;
		nextBlockWaitDuration_ =
			(desc_.boss && desc_.boss->IsEnraged())
			? enragedNextBlockWaitDuration_
			: nextBlockWaitDuration_;

		currentBlock_ = blockQueue_[blockIndex_];
	}

	// 待機が終わったら現在ブロック開始
	if (queueInited_ && !blockStarted_ && !waitingNextBlock_) {
		if (blockIndex_ < blockQueue_.size()) {
			currentBlock_ = blockQueue_[blockIndex_];
			if (StartBlock(currentBlock_)) {
				++blockIndex_;
			}
		}
	}

	// 現在のブロックが終わったら次へ
	bool finished = false;
	switch (currentBlock_) {
	case BossAttackBlock::ArmCombo:
		finished = blockStarted_ && (desc_.boss && desc_.boss->ConsumeArmComboFinished());
		break;
	case BossAttackBlock::Charge:
		finished = blockStarted_ && (charge_ && !charge_->IsActive());
		break;
	case BossAttackBlock::Meteor:
		finished = blockStarted_ && (meteor_ && !meteor_->IsActive());
		break;
	}

	if (finished) {

		// 1周のキューを消化しきったら作り直してまたランダム
		if (blockIndex_ >= blockQueue_.size()) {
			RebuildBlockQueue();
		}

		// 次の攻撃まで待機
		waitingNextBlock_ = true;
		nextBlockWaitTimer_ = 0.0f;
		nextBlockWaitDuration_ =
			(desc_.boss && desc_.boss->IsEnraged())
			? enragedNextBlockWaitDuration_
			: nextBlockWaitDuration_;

		blockStarted_ = false;
	}
	// =============================================================== //
}

void BossAttackManager::OnCurrentAttackFinished() {}

void BossAttackManager::StartEnragePause(float duration) {

	enragePauseActive_ = true;
	enragePauseTimer_ = 0.0f;
	enragePauseDuration_ = duration;

	// ボス本体側の攻撃停止
	if (desc_.boss) {
		desc_.boss->CancelAllAttacks();
		desc_.boss->StartEnrageTransition(duration);
	}

	// 進行中メテオは止める
	if (meteor_ && meteor_->IsActive()) {
		meteor_->ForceEnd();
	}

	// 離脱も止める
	if (retreat_ && retreat_->IsActive()) {
		retreat_->ForceEnd();
	}

	// ミサイルも止める
	if (missile_ && missile_->IsActive()) {
		missile_->ForceEnd();
	}

	// 今のブロックは未開始扱いにして、再開後に改めて始める
	blockStarted_ = false;

	waitingNextBlock_ = true;
	nextBlockWaitTimer_ = 0.0f;
	nextBlockWaitDuration_ = enragedNextBlockWaitDuration_;
}

void BossAttackManager::StartMeteor() {

	if (!meteor_) return;
	if (!meteor_->IsActive()) {
		meteor_->Start();
	}
}

void BossAttackManager::ForceEndMeteor() {

	if (!meteor_) return;
	if (meteor_->IsActive()) {
		meteor_->ForceEnd();
	}
}

void BossAttackManager::RebuildBlockQueue() {

	blockQueue_.clear();
	blockIndex_ = 0;

	// まずは「腕塊 + チャージ + メテオ」を1周分としてシャッフル
	blockQueue_.push_back(BossAttackBlock::ArmCombo);
	blockQueue_.push_back(BossAttackBlock::Charge);
	blockQueue_.push_back(BossAttackBlock::Meteor);

	// ランダム化（ブロック順だけ）
	std::shuffle(blockQueue_.begin(), blockQueue_.end(), rng_);

	queueInited_ = true;
}

bool BossAttackManager::IsBlockActive(BossAttackBlock b) const {

	switch (b) {
	case BossAttackBlock::ArmCombo:
		return desc_.boss && desc_.boss->IsArmComboActive();
	case BossAttackBlock::Charge:
		return charge_ && charge_->IsActive();
	case BossAttackBlock::Meteor:
		return meteor_ && meteor_->IsActive();
	default:
		return false;
	}
}

bool BossAttackManager::StartBlock(BossAttackBlock b) {

	currentBlock_ = b;
	blockStarted_ = false;

	if (!desc_.boss) return false;

	// 退避中は開始しない
	if (desc_.boss->IsRetreating()) {
		return false;
	}

	switch (b) {

	case BossAttackBlock::ArmCombo:
		if (!IsMeteorActive() && !IsChargeActive()) {
			desc_.boss->StartArmCombo();
			blockStarted_ = true;
			return true;
		}
		break;

	case BossAttackBlock::Charge:
		if (!IsMeteorActive() &&
			charge_ && !charge_->IsActive()) {

			charge_->Start();
			blockStarted_ = true;
			return true;
		}
		break;

	case BossAttackBlock::Meteor:
		if (!IsChargeActive() &&
			meteor_ && !meteor_->IsActive()) {

			meteor_->Start();
			blockStarted_ = true;
			return true;
		}
		break;

	default:
		break;
	}

	return false;
}

void BossAttackManager::StopAllAttacks(float dt) {

	// ==============================
	// デバッグ用：全攻撃停止
	// ==============================
	if (debugPauseAllAttacks_) {

		if (desc_.boss) {
			desc_.boss->CancelAllAttacks();
		}

		if (meteor_ && meteor_->IsActive()) {
			meteor_->ForceEnd();
		}
		if (charge_ && charge_->IsActive()) {
			charge_->ForceEnd();
		}
		if (retreat_ && retreat_->IsActive()) {
			retreat_->ForceEnd();
		}
		if (missile_ && missile_->IsActive()) {
			missile_->ForceEnd();
		}

		// 腕カメラ演出だけ残したくないので false 更新だけ
		if (arm_) {
			arm_->Update(dt, false);
		}

		// 自動攻撃再開を防ぐ
		blockStarted_ = false;

		return;
	}
}

void BossAttackManager::SetDebugPauseAllAttacks(bool pause) {

	debugPauseAllAttacks_ = pause;

	if (!pause) {
		return;
	}

	// pause を入れた瞬間にも止める
	if (desc_.boss) {
		desc_.boss->CancelAllAttacks();
	}

	if (meteor_ && meteor_->IsActive()) {
		meteor_->ForceEnd();
	}
	if (charge_ && charge_->IsActive()) {
		charge_->ForceEnd();
	}
	if (retreat_ && retreat_->IsActive()) {
		retreat_->ForceEnd();
	}
	if (missile_ && missile_->IsActive()) {
		missile_->ForceEnd();
	}

	blockStarted_ = false;
}

void BossAttackManager::RequestDebugChargeAttack(bool targetLeft) {

	if (!desc_.boss) {
		return;
	}

	// まず他の攻撃を全部止める
	SetDebugPauseAllAttacks(false);

	desc_.boss->CancelAllAttacks();

	if (meteor_ && meteor_->IsActive()) {
		meteor_->ForceEnd();
	}
	if (charge_ && charge_->IsActive()) {
		charge_->ForceEnd();
	}
	if (retreat_ && retreat_->IsActive()) {
		retreat_->ForceEnd();
	}
	if (missile_ && missile_->IsActive()) {
		missile_->ForceEnd();
	}

	// 次フレームの自動開始判定が暴れないようにする
	blockStarted_ = false;

	// ボスにチャージ要求だけ積む
	desc_.boss->RequestChargeAttack(targetLeft);
}

bool BossAttackManager::IsMeteorActive() const {
	return meteor_ && meteor_->IsActive();
}

bool BossAttackManager::IsChargeActive() const {
	return charge_ && charge_->IsActive();
}

bool BossAttackManager::IsAnyAttackActive() const {
	return IsMeteorActive()
		|| IsChargeActive()
		|| (arm_ && arm_->IsActive())
		|| (missile_ && missile_->IsActive())
		|| (retreat_ && retreat_->IsActive());
}