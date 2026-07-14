#include "BossChargeEffectController.h"

#include "Engine/Base/System/System.h"
#include "Engine/Base/Particle/ParticleManager.h"

void BossChargeEffectController::Update(
	float dt,
	bool chargeActive,
	const Vector3& effectPos
) {

	auto* pm = KomEngine::System::GetParticleManager();
	if (!pm) {
		return;
	}

	if (!chargeActive) {
		Reset();
		return;
	}

	const bool hasCore = pm->Exists("charge_core");
	const bool hasPulse = pm->Exists("charge_pulse");
	const bool hasAura = pm->Exists("charge_aura");
	const bool hasLine = pm->Exists("player_charge_line");

	// 青白い気の色に統一
	pm->SetChargeEffectColor(
		{ 0.72f, 0.90f, 1.00f, 1.0f },
		{ 0.85f, 0.95f, 1.00f, 1.0f }
	);

	// 中心に吸い込まれる細かい粒
	coreTimer_ += dt;
	if (coreTimer_ >= 0.035f) {
		coreTimer_ = 0.0f;

		if (hasCore) {
			pm->Emit("charge_core", effectPos, 12);
		}
	}

	// 遠くから中心に集まる線
	ribbonTimer_ += dt;
	if (ribbonTimer_ >= 0.060f) {
		ribbonTimer_ = 0.0f;

		if (hasLine) {
			pm->Emit("player_charge_line", effectPos, 5);
		}
	}

	// 外周の脈動リング
	pulseTimer_ += dt;
	if (pulseTimer_ >= 0.22f) {
		pulseTimer_ = 0.0f;

		if (hasPulse) {
			pm->Emit("charge_pulse", effectPos, 1);
		}
	}

	// moonLight はチャージ演出では使わないのでタイマーだけリセット
	ringTimer_ = 0.0f;

	// 中心の大きい“気の塊”本体
	cylinderTimer_ += dt;
	if (cylinderTimer_ >= 0.18f) {
		cylinderTimer_ = 0.0f;

		if (hasAura) {
			pm->Emit("charge_aura", effectPos, 2);
		}
	}
}

void BossChargeEffectController::Reset() {

	coreTimer_ = 0.0f;
	pulseTimer_ = 0.0f;
	ribbonTimer_ = 0.0f;
	ringTimer_ = 0.0f;
	cylinderTimer_ = 0.0f;
}