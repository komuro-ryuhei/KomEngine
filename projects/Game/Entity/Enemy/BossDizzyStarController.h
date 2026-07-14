#pragma once

#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/lib/Math/MyMath.h"
#include "struct.h"

#include <array>
#include <memory>

class Camera;

class BossDizzyStarController {

public:

	void Init(Camera* camera);

	void SetActive(bool active);
	bool IsActive() const { return active_; }

	void Update(float dt, const Vector3& bossPos);
	void Draw();

private:

	struct DizzyStar {
		std::unique_ptr<Object3d> obj;
		float angle = 0.0f;
		float phaseOffset = 0.0f;
	};

private:

	std::array<DizzyStar, 4> stars_;

	bool active_ = false;
	float timer_ = 0.0f;

	float orbitRadiusX_ = 3.2f;
	float orbitRadiusZ_ = 1.2f;
	float height_ = 2.4f;
	float orbitSpeed_ = 2.0f;
	float floatAmp_ = 0.25f;
	float floatSpeed_ = 3.0f;
	float starScale_ = 0.1f;
};