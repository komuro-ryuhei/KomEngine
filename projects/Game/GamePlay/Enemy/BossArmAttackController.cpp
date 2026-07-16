#include "BossArmAttackController.h"

#include "Game/Entity/Enemy/BossEnemy.h"
#include "Game/Entity/Player/Player.h"

#include "Engine/Base/System/System.h"
#include "Engine/Base/Particle/ParticleManager.h"
#include "Engine/lib/Math/MyMath.h"

#include <algorithm>
#include <cmath>

void BossArmAttackController::UpdateSingleArm(

	BossEnemy& boss,
	float dt
) {

	if (!boss.player_) {
		return;
	}

	auto* particleManager =
		KomEngine::System::GetParticleManager();

	// 現在フェーズが左腕かどうか
	const bool useLeft =
		boss.attackPhase_ ==
		BossEnemy::AttackPhase::SingleLeft;

	Object3d* targetArm =
		useLeft
		? boss.leftArm_.get()
		: boss.rightArm_.get();

	if (!targetArm) {
		return;
	}

	Vector3& targetPos =
		useLeft
		? boss.leftArmPos_
		: boss.rightArmPos_;

	int& hitCount =
		useLeft
		? boss.leftArmHitCount_
		: boss.rightArmHitCount_;

	// 腕の通常位置
	const Vector3 baseLocalOffset =
		useLeft
		? Vector3{ -4.0f, 0.0f, 0.0f }
	: Vector3{ 4.0f, 0.0f, 0.0f };

	Vector3 armPos = targetPos;

	// プレイヤー方向を取得
	const Vector3 worldBase =
		boss.transform_.translate + baseLocalOffset;

	const Vector3 toPlayer =
		boss.player_->GetTranslate() - worldBase;

	const Vector3 direction =
		MyMath::Normalize(toPlayer);

	// =====================================================
	// 予備動作
	// =====================================================
	if (boss.armTelegraphActive_) {

		// 開始直後だけ初期位置を保存
		if (boss.armTelegraphTimer_ <= 0.0f) {

			boss.armTelegraphStartPos_ =
				baseLocalOffset;

			boss.armTelegraphTargetPos_ =
				baseLocalOffset -
				direction * boss.armTelegraphBackAmount_;

			targetPos =
				boss.armTelegraphStartPos_;

			targetArm->SetTranslate(targetPos);
		}

		boss.armTelegraphTimer_ += dt;

		Vector3 telegraphPos =
			boss.armTelegraphStartPos_;

		// 通常位置から後ろへ引く
		if (boss.armTelegraphTimer_ <
			boss.armTelegraphBackTime_) {

			float t =
				boss.armTelegraphTimer_ /
				boss.armTelegraphBackTime_;

			t = std::clamp(t, 0.0f, 1.0f);

			const float ease =
				t * t * (3.0f - 2.0f * t);

			telegraphPos = MyMath::Lerp(
				boss.armTelegraphStartPos_,
				boss.armTelegraphTargetPos_,
				ease
			);
		}
		// 引いた位置で小刻みに振動
		else {

			const float shakeTime =
				boss.armTelegraphTimer_ -
				boss.armTelegraphBackTime_;

			telegraphPos =
				boss.armTelegraphTargetPos_;

			telegraphPos.x +=
				std::sin(
					shakeTime *
					boss.armTelegraphShakeFreq_
				) * boss.armTelegraphShakeAmount_;

			telegraphPos.y +=
				std::cos(
					shakeTime *
					boss.armTelegraphShakeFreq_ *
					1.11f
				) * boss.armTelegraphShakeAmount_;

			telegraphPos.z +=
				std::sin(
					shakeTime *
					boss.armTelegraphShakeFreq_ *
					0.91f
				) * boss.armTelegraphShakeAmount_;
		}

		armPos = telegraphPos;
		targetPos = armPos;

		targetArm->SetTranslate(targetPos);

		// 予備動作終了
		if (boss.armTelegraphTimer_ >=
			boss.armTelegraphDuration_) {

			boss.armTelegraphActive_ = false;
			boss.armTelegraphTimer_ = 0.0f;

			boss.isExtending_ = true;

			targetPos =
				boss.armTelegraphTargetPos_;

			targetArm->SetTranslate(targetPos);
		}

		return;
	}

	// =====================================================
	// 腕を伸ばす
	// =====================================================
	if (boss.isExtending_) {

		armPos +=
			direction * boss.armRushSpeed_;

		// 風切りエフェクト
		boss.armWindSlashFxTimer_ += dt;

		if (particleManager &&
			boss.armWindSlashFxTimer_ >=
			boss.armWindSlashFxInterval_) {

			boss.armWindSlashFxTimer_ = 0.0f;

			const Vector3 armWorldPos =
				boss.transform_.translate + armPos;

			particleManager->EmitArmWindSlash(
				armWorldPos,
				direction,
				2
			);
		}

		const float extensionLength =
			MyMath::Length(
				armPos - baseLocalOffset
			);

		const bool reachedMaxDistance =
			extensionLength >= 22.0f;

		const bool reachedMaxHit =
			hitCount >= boss.maxHitCount_;

		if (reachedMaxDistance || reachedMaxHit) {
			boss.isExtending_ = false;
		}
	}
	// =====================================================
	// 元の位置へ戻す
	// =====================================================
	else {

		const Vector3 toOrigin =
			baseLocalOffset - armPos;

		const float distance =
			MyMath::Length(toOrigin);

		if (distance < 0.5f) {

			armPos = baseLocalOffset;
			hitCount = 0;

			// 右腕終了後は左腕へ
			if (boss.attackPhase_ ==
				BossEnemy::AttackPhase::SingleRight) {

				boss.StartSingleArmPhase(
					BossEnemy::AttackPhase::SingleLeft
				);
			}
			// 左腕終了後は両手攻撃へ
			else {
				boss.StartBothHandsPhase();
			}
		}
		else {

			const Vector3 returnDirection =
				MyMath::Normalize(toOrigin);

			const float step =
				std::min(
					boss.armReturnSpeedSingle_,
					distance
				);

			armPos += returnDirection * step;
		}
	}

	targetArm->SetTranslate(armPos);
	targetPos = armPos;
}

void BossArmAttackController::UpdateBothHands(
	BossEnemy& boss,
	float dt
) {

	if (!boss.player_ ||
		!boss.leftArm_ ||
		!boss.rightArm_) {
		return;
	}

	auto* particleManager =
		KomEngine::System::GetParticleManager();

	const Vector3 leftBaseLocal{
		-4.0f, 0.0f, 0.0f
	};

	const Vector3 rightBaseLocal{
		4.0f, 0.0f, 0.0f
	};

	const Vector3 leftBaseWorld =
		boss.transform_.translate + leftBaseLocal;

	const Vector3 rightBaseWorld =
		boss.transform_.translate + rightBaseLocal;

	Vector3 leftWorldPos =
		boss.leftArm_->GetWorldPosition();

	Vector3 rightWorldPos =
		boss.rightArm_->GetWorldPosition();

	const Vector3 playerPos =
		boss.player_->GetTranslate();

	const Vector3 leftDirection =
		MyMath::Normalize(
			playerPos - leftBaseWorld
		);

	const Vector3 rightDirection =
		MyMath::Normalize(
			playerPos - rightBaseWorld
		);

	const float maxLength = 22.0f;
	const float returnSpeed =
		boss.armReturnSpeedBoth_;

	const float endThreshold = 0.3f;

	// =====================================================
	// 両手攻撃の予備動作
	// =====================================================
	if (boss.bothTelegraphActive_) {

		if (boss.bothTelegraphTimer_ <= 0.0f) {

			boss.leftBothTelegraphStartPos_ =
				leftBaseLocal;

			boss.rightBothTelegraphStartPos_ =
				rightBaseLocal;

			boss.leftBothTelegraphTargetPos_ =
				leftBaseLocal -
				leftDirection *
				boss.bothTelegraphBackAmount_;

			boss.rightBothTelegraphTargetPos_ =
				rightBaseLocal -
				rightDirection *
				boss.bothTelegraphBackAmount_;

			boss.leftArmPos_ =
				boss.leftBothTelegraphStartPos_;

			boss.rightArmPos_ =
				boss.rightBothTelegraphStartPos_;

			boss.leftArm_->SetTranslate(
				boss.leftArmPos_
			);

			boss.rightArm_->SetTranslate(
				boss.rightArmPos_
			);
		}

		boss.bothTelegraphTimer_ += dt;

		Vector3 leftLocal =
			boss.leftBothTelegraphStartPos_;

		Vector3 rightLocal =
			boss.rightBothTelegraphStartPos_;

		// 後ろへ引く
		if (boss.bothTelegraphTimer_ <
			boss.bothTelegraphBackTime_) {

			float t =
				boss.bothTelegraphTimer_ /
				boss.bothTelegraphBackTime_;

			t = std::clamp(t, 0.0f, 1.0f);

			const float ease =
				t * t * (3.0f - 2.0f * t);

			leftLocal = MyMath::Lerp(
				boss.leftBothTelegraphStartPos_,
				boss.leftBothTelegraphTargetPos_,
				ease
			);

			rightLocal = MyMath::Lerp(
				boss.rightBothTelegraphStartPos_,
				boss.rightBothTelegraphTargetPos_,
				ease
			);
		}
		// 引いた位置で振動
		else {

			const float shakeTime =
				boss.bothTelegraphTimer_ -
				boss.bothTelegraphBackTime_;

			leftLocal =
				boss.leftBothTelegraphTargetPos_;

			rightLocal =
				boss.rightBothTelegraphTargetPos_;

			const float shakeX =
				std::sin(
					shakeTime *
					boss.bothTelegraphShakeFreq_
				) * boss.bothTelegraphShakeAmount_;

			const float shakeY =
				std::cos(
					shakeTime *
					boss.bothTelegraphShakeFreq_ *
					1.09f
				) * boss.bothTelegraphShakeAmount_;

			const float shakeZ =
				std::sin(
					shakeTime *
					boss.bothTelegraphShakeFreq_ *
					0.93f
				) * boss.bothTelegraphShakeAmount_;

			leftLocal.x += shakeX;
			leftLocal.y += shakeY;
			leftLocal.z += shakeZ;

			rightLocal.x -= shakeX;
			rightLocal.y += shakeY;
			rightLocal.z += shakeZ;
		}

		boss.leftArm_->SetTranslate(leftLocal);
		boss.rightArm_->SetTranslate(rightLocal);

		boss.leftArmPos_ = leftLocal;
		boss.rightArmPos_ = rightLocal;

		if (boss.bothTelegraphTimer_ >=
			boss.bothTelegraphDuration_) {

			boss.bothTelegraphActive_ = false;
			boss.bothTelegraphTimer_ = 0.0f;

			boss.leftExtending_ = true;
			boss.rightExtending_ = true;

			boss.leftArmPos_ =
				boss.leftBothTelegraphTargetPos_;

			boss.rightArmPos_ =
				boss.rightBothTelegraphTargetPos_;

			boss.leftArm_->SetTranslate(
				boss.leftArmPos_
			);

			boss.rightArm_->SetTranslate(
				boss.rightArmPos_
			);
		}

		return;
	}

	// =====================================================
	// 左腕の伸縮
	// =====================================================
	if (boss.leftExtending_) {

		leftWorldPos +=
			leftDirection *
			boss.bothRushSpeed_;

		const float length =
			MyMath::Length(
				leftWorldPos - leftBaseWorld
			);

		const bool reachedDistance =
			length >= maxLength;

		const bool reachedHit =
			boss.leftArmHitCount_ >=
			boss.maxHitCount_;

		if (reachedDistance || reachedHit) {
			boss.leftExtending_ = false;
		}
	}
	else {

		const Vector3 toBase =
			leftBaseWorld - leftWorldPos;

		const float distance =
			MyMath::Length(toBase);

		if (distance < endThreshold) {
			leftWorldPos = leftBaseWorld;
		}
		else {

			const Vector3 returnDirection =
				MyMath::Normalize(toBase);

			const float step =
				std::min(returnSpeed, distance);

			leftWorldPos +=
				returnDirection * step;
		}
	}

	// =====================================================
	// 右腕の伸縮
	// =====================================================
	if (boss.rightExtending_) {

		rightWorldPos +=
			rightDirection *
			boss.bothRushSpeed_;

		const float length =
			MyMath::Length(
				rightWorldPos - rightBaseWorld
			);

		const bool reachedDistance =
			length >= maxLength;

		const bool reachedHit =
			boss.rightArmHitCount_ >=
			boss.maxHitCount_;

		if (reachedDistance || reachedHit) {
			boss.rightExtending_ = false;
		}
	}
	else {

		const Vector3 toBase =
			rightBaseWorld - rightWorldPos;

		const float distance =
			MyMath::Length(toBase);

		if (distance < endThreshold) {
			rightWorldPos = rightBaseWorld;
		}
		else {

			const Vector3 returnDirection =
				MyMath::Normalize(toBase);

			const float step =
				std::min(returnSpeed, distance);

			rightWorldPos +=
				returnDirection * step;
		}
	}

	// =====================================================
	// 風切りエフェクト
	// =====================================================
	if (particleManager &&
		(boss.leftExtending_ ||
			boss.rightExtending_)) {

		boss.armWindSlashFxTimer_ += dt;

		if (boss.armWindSlashFxTimer_ >=
			boss.armWindSlashFxInterval_) {

			boss.armWindSlashFxTimer_ = 0.0f;

			if (boss.leftExtending_) {

				particleManager->EmitArmWindSlash(
					leftWorldPos,
					leftDirection,
					2
				);
			}

			if (boss.rightExtending_) {

				particleManager->EmitArmWindSlash(
					rightWorldPos,
					rightDirection,
					2
				);
			}
		}
	}

	// ワールド座標からローカル座標へ戻す
	const Vector3 leftLocal =
		leftWorldPos - boss.transform_.translate;

	const Vector3 rightLocal =
		rightWorldPos - boss.transform_.translate;

	boss.leftArm_->SetTranslate(leftLocal);
	boss.rightArm_->SetTranslate(rightLocal);

	boss.leftArmPos_ = leftLocal;
	boss.rightArmPos_ = rightLocal;

	const bool leftFinished =
		!boss.leftExtending_ &&
		MyMath::Length(
			leftWorldPos - leftBaseWorld
		) < endThreshold;

	const bool rightFinished =
		!boss.rightExtending_ &&
		MyMath::Length(
			rightWorldPos - rightBaseWorld
		) < endThreshold;

	// 両腕が戻ったらコンボ終了
	if (leftFinished && rightFinished) {
		boss.FinishArmCombo();
	}
}