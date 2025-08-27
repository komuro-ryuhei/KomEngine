#pragma once

#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/lib/Input/Input.h"
#include "struct.h"
#include <vector>

#include "Game/Entity/Enemy/EnemyBullet.h"
#include "Game/Entity/Player/Player.h"

class Player;

class Enemy {
public:
	void Init(Camera* camera, Object3d* object3d);
	void Update();
	void Draw();
	void ImGuiDebug();

	void StartJump(const Vector3& start, const Vector3& target, float speedXZ = 0.35f, float initialVy = 0.28f);

public:
	Vector3 GetTranslate();
	float GetRadius() const;
	bool GetIsAlive() const;
	void OnHit();
	std::vector<std::unique_ptr<EnemyBullet>>& GetBullets();

	void SetTranslate(Vector3 translate);
	void SetPlayer(Player* player);
	void SetRadius(float radius);
	void SetGravity(float g) { jumpParams_.gravity_ = g; }

private:
	void Move();
	void Attack();

private:
	Camera* camera_ = nullptr;
	Object3d* object3d_ = nullptr;
	Transform transform_;
	Player* player_ = nullptr; // プレイヤー参照用

	float velocity_ = 0.05f;
	float radius_ = 1.0f;
	bool isAlive_ = true;

	// 点滅用の変数
	bool isBlinking_ = false;
	int blinkCounter_ = 0;
	const int blinkDuration_ = 60;

	// 
	std::vector<std::unique_ptr<EnemyBullet>> bulletObjects_;
	std::vector<std::unique_ptr<Object3d>> bulletObject3ds_;
	int attackTimer_ = 0;

	int attackInterval_ = 0; // 2秒に1回くらい

	// 出現してからの待機時間
	int spawnWaitTimer_ = 0;
	const int spawnWaitDuration_ = 180; // 180フレーム = 3秒（60FPS基準）

	// ジャンプに必要な処理
	struct JumpParams {
		bool isJumping_ = false;
		float gravity_ = -0.012f; // 調整用: 放物線の“落ち”の強さ
		Vector3 start_{ 0,0,0 };  // 開始位置
		Vector3 target_{ 0,0,0 }; // 着地点(最終ターゲット)
		int framesTotal_ = 0;     // 到達までの総フレーム T
		int framesElapsed_ = 0;   // 経過フレーム k
		float v0y_ = 0.0f;        // 垂直初速( T フレームで y=0 に戻す )
	};
	JumpParams jumpParams_;
};