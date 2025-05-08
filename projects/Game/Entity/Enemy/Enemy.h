#pragma once

#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/lib/Input/Input.h"
#include "struct.h"
#include <vector>

class Enemy {
public:
	void Init(Camera* camera, Object3d* object3d);
	void Update();
	void Draw();
	void ImGuiDebug();

	Vector3 GetTranslate();
	void SetTranslate(Vector3 translate);
	float GetRadius() const;
	bool GetIsAlive() const;
	void OnHit();

private:
	void Move();
	void SetRandomDirection(); // ランダムな方向を決定
	void CheckBounds();        // 範囲外チェック

private:
	Camera* camera_ = nullptr;
	Object3d* object3d_ = nullptr;
	Transform transform_;

	float velocity_ = 0.05f;
	float radius_ = 1.0f;
	bool isAlive_ = true;

	// 点滅用の変数
	bool isBlinking_ = false;
	int blinkCounter_ = 0;
	const int blinkDuration_ = 60;

	// ランダム移動用
	Vector3 moveDirection_;        // 移動方向
	int moveTimer_ = 0;            // 移動タイマー
	const int moveDuration_ = 300; // 5秒（60FPS * 5）

	// 移動可能範囲
	const float minX = -5.0f, maxX = 5.0f;
	const float minY = -5.0f, maxY = 5.0f;
	const float minZ = 20.0f, maxZ = 50.0f;
};