// BossMeteor.h
#pragma once
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Collision/ICollisionObject.h"
#include "Game/Entity/Player/Player.h"

class Camera;
class Player;

// ボスが落とす隕石。スポーン→落下（重力）→着弾で爆発→消滅 という最小機能。
class BossMeteor : public ICollisionObject {

public:

	BossMeteor() = default;
	~BossMeteor() = default;

	// 必須初期化：カメラ共有（描画用）と内部Object3d生成
	void Init(Camera* camera);

	// フレーム更新／描画
	void Update();
	void Draw();

	// デバッグUI
	void ImGuiDebug();

	// 生成（再利用前提）。startPos から targetPos 方向へ初速を与える
	void Spawn(const Vector3& startPos, const Vector3& targetPos, float speed = 0.6f);

	// 外部から強制爆発させたいとき
	void Explode();

	// ライフサイクル
	bool IsAlive() const { return isAlive_; }

	// 取得系
	float GetRadius() const { return radius_; }
	const Transform& GetTransform() const { return transform_; }
	Object3d* GetObject() const { return object3d_.get(); }

	// 調整系
	void SetRadius(float r) { radius_ = r; if (object3d_) object3d_->SetRadius(radius_ * object3d_->GetScale().x); }
	void SetScale(const Vector3& s) { if (object3d_) { object3d_->SetScale(s); object3d_->SetRadius(radius_ * s.x); } }
	void SetGravity(float g) { gravity_ = g; accel_ = { 0.0f, -gravity_, 0.0f }; }

	// ----------------------- ICollisionObjectの実装 ----------------------- //
	Vector3 GetCollisionPosition() const override;
	float   GetCollisionRadius() const override;
	CollisionLayer GetCollisionLayer() const override { return CollisionLayer::EnemyBullet; }
	void OnCollision(ICollisionObject* other) override;

private:

	void ApplyPhysics();
	void OnHitGround();

private:

	// 描画
	Camera* camera_ = nullptr;
	std::unique_ptr<Object3d> object3d_ = nullptr;

	// 物理／状態
	Transform transform_{};         // 平行移動はここがソース
	Vector3 velocity_{};            // 初速＋更新で使用
	Vector3 accel_{};               // 重力のみ使用
	float gravity_ = 0.02f;         // 下向き加速度
	float rotateSpeed_ = 0.05f;     // くるくる回転
	float radius_ = 1.2f;           // 当たり判定（スケール前ベース）
	bool  isAlive_ = false;
	bool  isExploding_ = false;
	float lifeTimer_ = 0.0f;        // 生存時間
	float maxLife_ = 15.0f;         // 保険で自動消滅

	// 地面レベル（暫定）。シーン側で合わせるならsetter用意してもOK
	float groundY_ = 0.0f;
};