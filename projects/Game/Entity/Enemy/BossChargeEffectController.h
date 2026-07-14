#pragma once

#include "struct.h"
#include "Engine/lib/Math/MyMath.h"

class BossChargeEffectController {

public:

	void Update(
		float dt,
		bool chargeActive,
		const Vector3& effectPos
	);

	void Reset();

private:

	float coreTimer_ = 0.0f;
	float pulseTimer_ = 0.0f;
	float ribbonTimer_ = 0.0f;
	float ringTimer_ = 0.0f;
	float cylinderTimer_ = 0.0f;
};