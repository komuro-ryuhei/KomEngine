#pragma once

// MyCalss
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/lib/Input/Input.h"
#include "struct.h"

class EnemyBullet {

	public:
	void Init(Camera* camera, Object3d* object3d);

	void Update();

	void Draw();

	void ImGuiDebug();

	float GetRadius() const;
	Vector3 GetTranslate() const;
	void SetTranlate(Vector3 translate);
	void SetDirection(const Vector3& direction);

private:
	Camera* camera_ = nullptr;

	Object3d* object3d_ = nullptr;

	Transform transform_;

	float speed_ = 0.003f;
	Vector3 direction_;

	float radius_ = 0.8f;
};