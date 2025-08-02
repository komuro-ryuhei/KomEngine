#pragma once
#include "Engine/Base/3d/Object3d/Object3d.h"

class Player;

class BossEnemy {

public:

	void Init(Camera* camera);

	void Update();

	void Draw();

	void ImGuiDebug();

public:

	// getter・setter
	float GetRadius() const;
	Transform GetTransform() const;
	void SetRotate(Vector3& rotate);
	void SetTranslate(Vector3 translate);
	void SetPlayer(Player* player) { player_ = player; }

public:
	Object3d* GetBody() const { return object3d_.get(); }
	Object3d* GetLeftArm() const { return leftArm_.get(); }
	Object3d* GetRightArm() const { return rightArm_.get(); }

private:

	void Attack();
	void Move();

public:
	void AddHitToAttackingArm();

private:
	// カメラ
	Camera* camera_ = nullptr;
	// SRT
	Transform transform_;
	// 半径
	float radius_ = 4.0f;

	Player* player_ = nullptr;

	// モデル
	std::unique_ptr<Object3d> object3d_ = nullptr;
	// 部位
	std::unique_ptr<Object3d> leftArm_;
	std::unique_ptr<Object3d> rightArm_;

	// 攻撃用のタイマーと状態
	float attackTimer_ = 0.0f;
	float attackInterval_ = 2.0f; // 2秒周期
	bool isExtending_ = true;     // 腕を伸ばしているか
	float attackCooldown_ = 0.0f;
	bool isAttacking_ = false;
	bool attackLeftArm_ = true;
	float attackSpeed_ = 0.2f; // 腕の伸縮速度
	int leftArmHitCount_ = 0;
	int rightArmHitCount_ = 0;
	const int maxHitCount_ = 5;
};