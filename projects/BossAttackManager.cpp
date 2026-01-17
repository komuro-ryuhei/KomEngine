#include "BossAttackManager.h"
#include "BossMeteorController.h"
#include "BossArmController.h"

#include "Game/Entity/Enemy/BossEnemy.h"

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
}

bool BossAttackManager::CanArmControlCamera(const UpdateFlags& flags) const {

	// 
	return
		!flags.koActive
		&& flags.isMainPhase
		&& !IsMeteorActive()
		&& !IsChargeActive()
		&& !flags.swordCamActive
		&& flags.isCameraFollowPlayer;
}

void BossAttackManager::Update(float dt, const UpdateFlags& flags) {

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

	// ===== メテオ開始条件 =====
	const bool canStartMeteor =
		!flags.koActive &&
		flags.isMainPhase &&
		!(meteor_ && meteor_->IsActive());

	if (canStartMeteor && desc_.boss && desc_.boss->ConsumeMeteorRequest()) {
		meteor_->Start();
	}

	// ===== チャージビーム開始条件 =====
	const bool canStartCharge =
		!flags.koActive &&
		flags.isMainPhase &&
		!(charge_ && charge_->IsActive()) &&
		!(meteor_ && meteor_->IsActive());

	if (canStartCharge && desc_.boss && desc_.boss->ConsumeChargeRequest()) {
		charge_->Start();
	}

	// メテオ更新
	if (meteor_ && meteor_->IsActive()) {
		meteor_->Update(dt);
	}

	// チャージ更新
	if (charge_ && charge_->IsActive()) {
		charge_->Update(dt);
	}

	// メテオが終わった瞬間にBossへ通知（ForceEndでもここで拾える）
	const bool isMeteorActive = (meteor_ && meteor_->IsActive());
	if (wasMeteorActive && !isMeteorActive && desc_.boss) {
		desc_.boss->OnMeteorFinished();
	}

	// チャージが終わった瞬間にBossへ通知
	const bool isChargeActive = (charge_ && charge_->IsActive());
	if (wasChargeActive && !isChargeActive && desc_.boss) {
		desc_.boss->OnChargeAttackFinished();
	}
}

void BossAttackManager::OnCurrentAttackFinished() {
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

bool BossAttackManager::IsMeteorActive() const {
	return meteor_ && meteor_->IsActive();
}

bool BossAttackManager::IsChargeActive() const {
	return charge_ && charge_->IsActive();
}

bool BossAttackManager::IsAnyAttackActive() const {
	return IsMeteorActive() || IsChargeActive() || (arm_ && arm_->IsActive());
}