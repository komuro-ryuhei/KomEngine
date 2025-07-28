#pragma once
#include "Engine/Base/3d/Object3d/Object3d.h"

class BossEnemy {

public:

	void Init(Camera* camera);

	void Update();

	void Draw();

	void ImGuiDebug();

public:
	float GetRadius() const;
	Transform GetTransform() const;
	void SetRotate(Vector3& rotate);

private:
	void Attack();
	void Move();

private:
	// カメラ
	Camera* camera_ = nullptr;
	// モデル
	std::unique_ptr<Object3d> object3d_ = nullptr;
	// SRT
	Transform transform_;
};