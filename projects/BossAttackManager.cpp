#include "BossAttackManager.h"
#include "BossMeteorController.h"
#include "BossArmController.h"

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

	// 前フレームの状態
	const bool wasArmActive = (arm_ && arm_->IsActive());

	// 腕更新（ここでActiveが変わる）
	if (arm_) {
		arm_->Update(dt, CanArmControlCamera(flags));
	}

	const bool isArmActive = (arm_ && arm_->IsActive());

	// 腕が「終わった瞬間」(true→false) にメテオ開始
	const bool armJustFinished = (wasArmActive && !isArmActive);

	if (armJustFinished) {
		// メテオを開始してよい条件（最低限）
		const bool canStartMeteor =
			!flags.koActive &&
			flags.isMainPhase &&
			!(meteor_ && meteor_->IsActive());

		if (canStartMeteor) {
			meteor_->Start();
		}
	}

	// メテオ更新
	if (meteor_ && meteor_->IsActive()) {
		meteor_->Update(dt);
	}
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