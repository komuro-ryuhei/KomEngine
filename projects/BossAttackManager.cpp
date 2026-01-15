#include "BossAttackManager.h"
#include "BossMeteorController.h"
#include "BossArmController.h"
#include "Game/Entity/Enemy/BossEnemy.h"

#include <cassert>

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
}

bool BossAttackManager::CanArmControlCamera(const UpdateFlags& flags) const {

	// 
	return
		!flags.koActive
		&& flags.isMainPhase
		&& !IsMeteorActive()
		&& !flags.swordCamActive
		&& flags.isCameraFollowPlayer;
}

void BossAttackManager::Update(float dt, const UpdateFlags& flags) {

	const bool wasMeteorActive = (meteor_ && meteor_->IsActive());

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

	// メテオ更新
	if (meteor_ && meteor_->IsActive()) {
		meteor_->Update(dt);
	}

	// メテオが終わった瞬間にBossへ通知（ForceEndでもここで拾える）
	const bool isMeteorActive = (meteor_ && meteor_->IsActive());
	if (wasMeteorActive && !isMeteorActive && desc_.boss) {
		desc_.boss->OnMeteorFinished();
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

bool BossAttackManager::IsAnyAttackActive() const {
	return IsMeteorActive() || (arm_ && arm_->IsActive());
}