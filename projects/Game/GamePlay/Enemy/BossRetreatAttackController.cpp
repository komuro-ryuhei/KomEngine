#include "BossRetreatAttackController.h"

#include "Game/Entity/Enemy/BossEnemy.h"
#include "Game/Entity/Enemy/BossRetreatAttack.h"
#include "Game/GamePlay/Enemy/BossMissileController.h"

void BossRetreatAttackController::Init() {
}

void BossRetreatAttackController::Start() {
	if (!boss_ || !retreat_) {
		return;
	}

	retreat_->Start(
		boss_->GetTranslate(),
		boss_->GetBody()->GetScale(),
		boss_->GetBaseArmScale()
	);

	boss_->SetRetreating(true);
	boss_->SetInvulnerable(true);

	// 開始直後は現在のボス位置を見る
	boss_->SetCameraFocusPos(retreat_->GetTranslate());

	ApplyToBoss();
}

void BossRetreatAttackController::Update(float dt) {
	if (!boss_ || !retreat_ || !retreat_->IsActive()) {
		return;
	}

	retreat_->Update(dt);
	ApplyToBoss();

	if (retreat_->ConsumeHoldEntered()) {
		if (missile_ && !missile_->IsActive()) {
			missile_->Start();
		}
	}

	if (retreat_->IsHolding()) {
		if (!missile_ || !missile_->IsActive()) {
			retreat_->RequestReturn();
		}
	}

	if (retreat_->ConsumeFinished()) {
		boss_->SetRetreating(false);
		boss_->SetInvulnerable(false);
		boss_->ClearRetreatVisualOverride();

		if (boss_->pendingChargeAfterRetreat_) {
			boss_->pendingChargeAfterRetreat_ = false;
			boss_->RequestChargeAttack(boss_->nextChargeTargetLeft_);
			boss_->nextChargeTargetLeft_ = !boss_->nextChargeTargetLeft_;
		}
	}
}

void BossRetreatAttackController::ForceEnd() {
	if (!boss_ || !retreat_) {
		return;
	}

	retreat_->ForceEnd();
	boss_->SetRetreating(false);
	boss_->SetInvulnerable(false);
	boss_->ClearRetreatVisualOverride();

	if (missile_ && missile_->IsActive()) {
		missile_->ForceEnd();
	}
}

bool BossRetreatAttackController::IsActive() const {
	return retreat_ && retreat_->IsActive();
}

void BossRetreatAttackController::ApplyToBoss() {
	if (!boss_ || !retreat_) {
		return;
	}

	const Vector3 bossPos = retreat_->GetTranslate();

	boss_->ApplyRetreatPose(
		bossPos,
		retreat_->GetBodyScale(),
		retreat_->GetArmScale()
	);

	boss_->SetInvulnerable(retreat_->IsInvulnerable());

	// カメラの注視点は「退避先」固定ではなく、現在のボス位置を見る
	boss_->SetCameraFocusPos(bossPos);
}