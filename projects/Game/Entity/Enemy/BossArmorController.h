#pragma once

#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/lib/Math/MyMath.h"

#include <memory>
#include <vector>

class BossArmorController {

public:

	void Init(Camera* camera, Object3d* parent);

	void Update(float dt);
	void Draw();

	void Reset(int hp = 3);
	bool Damage(int damage, const Vector3& bossPos);

	bool AreAllBroken() const;
	int GetAliveCount() const;

#ifdef USE_IMGUI
	void ImGuiDebug();
#endif

private:

	struct ArmorUnit {
		std::unique_ptr<Object3d> obj;
		bool alive = true;
		float angle = 0.0f;
		int hp = 3;
	};

private:

	void Rebuild();
	void BreakOne();

private:

	Camera* camera_ = nullptr;
	Object3d* parent_ = nullptr;

	std::vector<ArmorUnit> armors_;

	int armorInitialCount_ = 12;

	float armorOrbitRadius_ = 3.0f;
	float armorOrbitSpeed_ = 0.9f;
	float armorFloatAmp_ = 0.18f;
	float armorFloatSpeed_ = 1.6f;

	float armorTime_ = 0.0f;
	float armorGlobalAngle_ = 0.0f;

	Vector3 armorScale_ = { 0.3f, 0.3f, 0.3f };

	bool armorRebuildRequest_ = false;
};