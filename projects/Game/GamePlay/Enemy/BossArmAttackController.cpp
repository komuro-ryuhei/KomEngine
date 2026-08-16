#include "BossArmAttackController.h"

#include "Game/Entity/Enemy/BossEnemy.h"
#include "Game/Entity/Player/Player.h"

#include "Engine/Base/System/System.h"
#include "Engine/Base/Particle/ParticleManager.h"
#include "Engine/lib/Math/MyMath.h"

#include <algorithm>
#include <cmath>

void BossArmAttackController::BeginSingleArmTelegraph() {

	// 最初は攻撃を伸ばさず、
	// 予備動作から開始する
	isExtending_ = false;

	armTelegraphActive_ = true;
	armTelegraphTimer_ = 0.0f;

	// 両腕側は停止
	bothTelegraphActive_ = false;
	bothTelegraphTimer_ = 0.0f;

	// エフェクトタイマーもリセット
	armWindSlashFxTimer_ = 0.0f;
}

void BossArmAttackController::BeginBothHandsTelegraph() {

	// 片腕側は停止
	armTelegraphActive_ = false;
	armTelegraphTimer_ = 0.0f;

	// 両腕の予備動作開始
	bothTelegraphActive_ = true;
	bothTelegraphTimer_ = 0.0f;

	leftExtending_ = false;
	rightExtending_ = false;

	armWindSlashFxTimer_ = 0.0f;
}

void BossArmAttackController::FinishComboState() {

	// 次回の攻撃に備えて初期状態へ戻す
	leftExtending_ = true;
	rightExtending_ = true;

	armTelegraphActive_ = false;
	armTelegraphTimer_ = 0.0f;

	bothTelegraphActive_ = false;
	bothTelegraphTimer_ = 0.0f;

	armWindSlashFxTimer_ = 0.0f;
}

void BossArmAttackController::Reset() {

	FinishComboState();

	isExtending_ = true;
}

void BossArmAttackController::SetEnraged(
	bool enraged,
	float speedMultiplier
) {

	if (enraged) {

		armReturnSpeedSingle_ =
			baseArmReturnSpeedSingle_ *
			speedMultiplier;

		armReturnSpeedBoth_ =
			baseArmReturnSpeedBoth_ *
			speedMultiplier;
	}
	else {

		armReturnSpeedSingle_ =
			baseArmReturnSpeedSingle_;

		armReturnSpeedBoth_ =
			baseArmReturnSpeedBoth_;
	}
}

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
	if (armTelegraphActive_) {

		if (armTelegraphTimer_ <= 0.0f) {

			armTelegraphStartPos_ =
				baseLocalOffset;

			armTelegraphTargetPos_ =
				baseLocalOffset -
				direction *
				armTelegraphBackAmount_;

			targetPos =
				armTelegraphStartPos_;

			targetArm->SetTranslate(targetPos);
		}

		armTelegraphTimer_ += dt;

		Vector3 telegraphPos =
			armTelegraphStartPos_;

		if (armTelegraphTimer_ <
			armTelegraphBackTime_) {

			float t =
				armTelegraphTimer_ /
				armTelegraphBackTime_;

			t = std::clamp(
				t,
				0.0f,
				1.0f
			);

			const float ease =
				t * t * (3.0f - 2.0f * t);

			telegraphPos =
				MyMath::Lerp(
					armTelegraphStartPos_,
					armTelegraphTargetPos_,
					ease
				);
		}
		else {

			const float shakeTime =
				armTelegraphTimer_ -
				armTelegraphBackTime_;

			telegraphPos =
				armTelegraphTargetPos_;

			telegraphPos.x +=
				std::sin(
					shakeTime *
					armTelegraphShakeFreq_
				) *
				armTelegraphShakeAmount_;

			telegraphPos.y +=
				std::cos(
					shakeTime *
					armTelegraphShakeFreq_ *
					1.11f
				) *
				armTelegraphShakeAmount_;

			telegraphPos.z +=
				std::sin(
					shakeTime *
					armTelegraphShakeFreq_ *
					0.91f
				) *
				armTelegraphShakeAmount_;
		}

		armPos = telegraphPos;
		targetPos = armPos;

		targetArm->SetTranslate(targetPos);

		// 予備動作終了
		if (armTelegraphTimer_ >=
			armTelegraphDuration_) {

			armTelegraphActive_ = false;
			armTelegraphTimer_ = 0.0f;

			isExtending_ = true;

			targetPos =
				armTelegraphTargetPos_;

			targetArm->SetTranslate(targetPos);
		}

		return;
	}

	// =====================================================
	// 腕を伸ばす
	// =====================================================
	if (isExtending_) {

		armPos += direction * armRushSpeed_;

		// 風切りエフェクト
		armWindSlashFxTimer_ += dt;

		if (particleManager &&
			armWindSlashFxTimer_ >=
			armWindSlashFxInterval_) {

			armWindSlashFxTimer_ = 0.0f;

			const Vector3 armWorldPos =
				boss.transform_.translate +
				armPos;

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
			isExtending_ = false;
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
					armReturnSpeedSingle_,
					distance
				);

			armPos += returnDirection * step;
		}
	}

	targetArm->SetTranslate(armPos);
	targetPos = armPos;
}

void BossArmAttackController::UpdateBothHands(BossEnemy& boss, float dt) {

	// プレイヤー、左右の腕が無ければ処理しない
	if (!boss.player_ ||
		!boss.leftArm_ ||
		!boss.rightArm_) {
		return;
	}

	auto* particleManager =
		KomEngine::System::GetParticleManager();

	// =====================================================
	// 腕の基準ローカル座標
	// =====================================================

	const Vector3 leftBaseLocal{
		-4.0f, 0.0f, 0.0f
	};

	const Vector3 rightBaseLocal{
		 4.0f, 0.0f, 0.0f
	};

	// ボス本体の位置を加えてワールド座標化
	const Vector3 leftBaseWorld =
		boss.transform_.translate +
		leftBaseLocal;

	const Vector3 rightBaseWorld =
		boss.transform_.translate +
		rightBaseLocal;

	// 現在の腕のワールド座標
	Vector3 leftWorldPos =
		boss.leftArm_->GetWorldPosition();

	Vector3 rightWorldPos =
		boss.rightArm_->GetWorldPosition();

	const Vector3 playerPos =
		boss.player_->GetTranslate();

	// プレイヤー方向
	const Vector3 leftDirection =
		MyMath::Normalize(
			playerPos -
			leftBaseWorld
		);

	const Vector3 rightDirection =
		MyMath::Normalize(
			playerPos -
			rightBaseWorld
		);

	// 最大伸長距離
	const float maxLength = 22.0f;

	// 戻り速度
	const float returnSpeed =
		armReturnSpeedBoth_;

	// 基準位置まで戻ったとみなす距離
	const float endThreshold = 0.3f;

	// =====================================================
	// 両手攻撃の予備動作
	// =====================================================

	if (bothTelegraphActive_) {

		// 予備動作開始時
		if (bothTelegraphTimer_ <= 0.0f) {

			leftBothTelegraphStartPos_ =
				leftBaseLocal;

			rightBothTelegraphStartPos_ =
				rightBaseLocal;

			// プレイヤーとは反対方向へ腕を引く
			leftBothTelegraphTargetPos_ =
				leftBaseLocal -
				leftDirection *
				bothTelegraphBackAmount_;

			rightBothTelegraphTargetPos_ =
				rightBaseLocal -
				rightDirection *
				bothTelegraphBackAmount_;

			boss.leftArmPos_ =
				leftBothTelegraphStartPos_;

			boss.rightArmPos_ =
				rightBothTelegraphStartPos_;

			boss.leftArm_->SetTranslate(
				boss.leftArmPos_
			);

			boss.rightArm_->SetTranslate(
				boss.rightArmPos_
			);
		}

		bothTelegraphTimer_ += dt;

		Vector3 leftLocal =
			leftBothTelegraphStartPos_;

		Vector3 rightLocal =
			rightBothTelegraphStartPos_;

		// =================================================
		// 腕を後ろへ引く
		// =================================================

		if (bothTelegraphTimer_ <
			bothTelegraphBackTime_) {

			float t =
				bothTelegraphTimer_ /
				bothTelegraphBackTime_;

			t = std::clamp(
				t,
				0.0f,
				1.0f
			);

			// SmoothStep
			const float ease =
				t * t *
				(3.0f - 2.0f * t);

			leftLocal =
				MyMath::Lerp(
					leftBothTelegraphStartPos_,
					leftBothTelegraphTargetPos_,
					ease
				);

			rightLocal =
				MyMath::Lerp(
					rightBothTelegraphStartPos_,
					rightBothTelegraphTargetPos_,
					ease
				);
		}

		// =================================================
		// 引いた位置で振動
		// =================================================

		else {

			const float shakeTime =
				bothTelegraphTimer_ -
				bothTelegraphBackTime_;

			leftLocal =
				leftBothTelegraphTargetPos_;

			rightLocal =
				rightBothTelegraphTargetPos_;

			const float shakeX =
				std::sin(
					shakeTime *
					bothTelegraphShakeFreq_
				) *
				bothTelegraphShakeAmount_;

			const float shakeY =
				std::cos(
					shakeTime *
					bothTelegraphShakeFreq_ *
					1.09f
				) *
				bothTelegraphShakeAmount_;

			const float shakeZ =
				std::sin(
					shakeTime *
					bothTelegraphShakeFreq_ *
					0.93f
				) *
				bothTelegraphShakeAmount_;

			// 左右でX方向の揺れを逆にする
			leftLocal.x += shakeX;
			leftLocal.y += shakeY;
			leftLocal.z += shakeZ;

			rightLocal.x -= shakeX;
			rightLocal.y += shakeY;
			rightLocal.z += shakeZ;
		}

		// 座標反映
		boss.leftArmPos_ =
			leftLocal;

		boss.rightArmPos_ =
			rightLocal;

		boss.leftArm_->SetTranslate(
			boss.leftArmPos_
		);

		boss.rightArm_->SetTranslate(
			boss.rightArmPos_
		);

		// =================================================
		// 予備動作終了
		// =================================================

		if (bothTelegraphTimer_ >=
			bothTelegraphDuration_) {

			bothTelegraphActive_ = false;
			bothTelegraphTimer_ = 0.0f;

			// ここから両腕を伸ばす
			leftExtending_ = true;
			rightExtending_ = true;

			boss.leftArmPos_ =
				leftBothTelegraphTargetPos_;

			boss.rightArmPos_ =
				rightBothTelegraphTargetPos_;

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
	// 左腕
	// =====================================================

	if (leftExtending_) {

		// プレイヤー方向へ伸ばす
		leftWorldPos +=
			leftDirection *
			bothRushSpeed_;

		const float length =
			MyMath::Length(
				leftWorldPos -
				leftBaseWorld
			);

		const bool reachedDistance =
			length >= maxLength;

		const bool reachedHit =
			boss.leftArmHitCount_ >=
			boss.maxHitCount_;

		// 最大距離または最大ヒット数で戻り開始
		if (reachedDistance ||
			reachedHit) {

			leftExtending_ = false;
		}
	}
	else {

		// 基準位置へ戻す
		const Vector3 toBase =
			leftBaseWorld -
			leftWorldPos;

		const float distance =
			MyMath::Length(toBase);

		if (distance <
			endThreshold) {

			leftWorldPos =
				leftBaseWorld;
		}
		else {

			const Vector3 returnDirection =
				MyMath::Normalize(toBase);

			const float step =
				std::min(
					returnSpeed,
					distance
				);

			leftWorldPos +=
				returnDirection *
				step;
		}
	}

	// =====================================================
	// 右腕
	// =====================================================

	if (rightExtending_) {

		// プレイヤー方向へ伸ばす
		rightWorldPos +=
			rightDirection *
			bothRushSpeed_;

		const float length =
			MyMath::Length(
				rightWorldPos -
				rightBaseWorld
			);

		const bool reachedDistance =
			length >= maxLength;

		const bool reachedHit =
			boss.rightArmHitCount_ >=
			boss.maxHitCount_;

		if (reachedDistance ||
			reachedHit) {

			rightExtending_ = false;
		}
	}
	else {

		// 基準位置へ戻す
		const Vector3 toBase =
			rightBaseWorld -
			rightWorldPos;

		const float distance =
			MyMath::Length(toBase);

		if (distance <
			endThreshold) {

			rightWorldPos =
				rightBaseWorld;
		}
		else {

			const Vector3 returnDirection =
				MyMath::Normalize(toBase);

			const float step =
				std::min(
					returnSpeed,
					distance
				);

			rightWorldPos +=
				returnDirection *
				step;
		}
	}

	// =====================================================
	// 風切りエフェクト
	// =====================================================

	if (particleManager &&
		(leftExtending_ ||
			rightExtending_)) {

		armWindSlashFxTimer_ += dt;

		if (armWindSlashFxTimer_ >=
			armWindSlashFxInterval_) {

			armWindSlashFxTimer_ = 0.0f;

			if (leftExtending_) {

				particleManager->
					EmitArmWindSlash(
						leftWorldPos,
						leftDirection,
						2
					);
			}

			if (rightExtending_) {

				particleManager->
					EmitArmWindSlash(
						rightWorldPos,
						rightDirection,
						2
					);
			}
		}
	}

	// =====================================================
	// ワールド座標 → ローカル座標
	// =====================================================

	const Vector3 leftLocal =
		leftWorldPos -
		boss.transform_.translate;

	const Vector3 rightLocal =
		rightWorldPos -
		boss.transform_.translate;

	boss.leftArmPos_ =
		leftLocal;

	boss.rightArmPos_ =
		rightLocal;

	boss.leftArm_->SetTranslate(
		boss.leftArmPos_
	);

	boss.rightArm_->SetTranslate(
		boss.rightArmPos_
	);

	// =====================================================
	// 攻撃終了判定
	// =====================================================

	const bool leftFinished =
		!leftExtending_ &&
		MyMath::Length(
			leftWorldPos -
			leftBaseWorld
		) <
		endThreshold;

	const bool rightFinished =
		!rightExtending_ &&
		MyMath::Length(
			rightWorldPos -
			rightBaseWorld
		) <
		endThreshold;

	// 両腕とも元の位置まで戻った
	if (leftFinished &&
		rightFinished) {

		boss.FinishArmCombo();
	}
}