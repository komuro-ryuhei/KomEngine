#pragma once

// MyCalss
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/lib/Input/Input.h"
#include "struct.h"

#include "Game/Entity/Player/PlayerBullet.h"
#include "Game/Entity/Enemy/Enemy.h"
#include "Engine/Base/2d/Sprite/Sprite.h"

// C++
#include <vector>
#include <algorithm>

class Player {

public:
	~Player();

	void Init(Camera* camera);

	void Update();

	void Draw();

	void ImGuiDebug();

public:
	float GetRadius() const;
	std::vector<std::unique_ptr<PlayerBullet>>& GetBullets();

private:
	void Attack();
	void Move();

	void UpdateReticleSprite();

private:
	// カメラ
	Camera* camera_ = nullptr;
	// モデル
	std::unique_ptr<Object3d> object3d_ = nullptr;
	// 弾のリスト
	std::vector<std::unique_ptr<PlayerBullet>> bulletObjects_;
	
	// レティクルのスプライト
	std::unique_ptr<Sprite> reticleSprite_ = nullptr;

	// SRT
	Transform transform_;

	// 速度
	float velocity_ = 0.05f;
	// 半径
	float radius_ = 1.0f;
	// 弾有効フラグ
	bool isBulletActive_ = false;
};