#pragma once

// MyCalss
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/lib/Input/Input.h"
#include "struct.h"

#include "Engine/Base/2d/Sprite/Sprite.h"
#include "Game/Entity/Player/PlayerBullet.h"

// C++
#include <algorithm>
#include <vector>

class Player {

public:
	~Player();

	void Init(Camera* camera);

	void Update();

	void Draw();

	void ImGuiDebug();

public:
	float GetRadius() const;
	Transform GetTransform() const;
	Vector3 GetTranslate() const;
	std::vector<std::unique_ptr<PlayerBullet>>& GetBullets();
	int GetHP() const;
	bool GetInvincible() const;

	void SetInvincible(bool flag);
	void SetRotate(Vector3& rotate);

	bool IsInvincible() const;
	void Damage(int amount);
	bool IsLowHP(int hp) const;

private:
	void Attack();

	void SpawnBullet();

	void UpdateReticleSprite();

public:
	void RailMove();
	void RotateY90();

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
	// HP
	int hp_ = 3;
	// 弾有効フラグ
	bool isBulletActive_ = false;

	// 無敵時間
	bool isInvincible_ = false;
	float invincibleTimer_ = 0.0f;

	// 連射制御（左クリック長押し用）
	float autofireInterval_ = 0.10f;
	float autofireTimer_ = 0.0f;
};