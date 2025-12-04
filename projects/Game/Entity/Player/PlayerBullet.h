#pragma once

// MyCalss
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/lib/Input/Input.h"
#include "Engine/Base/Particle/ParticleEmitter.h"
#include "struct.h"
#include "ICollisionObject.h"

class PlayerBullet : public ICollisionObject {

public:
	void Init(Camera* camera, Object3d* object3d);

	void Update();

	void Draw();

	void ImGuiDebug();

	float GetRadius() const;
	Vector3 GetTranslate() const;
	void SetTranlate(Vector3 translate);
	void SetDirection(const Vector3& direction);
	bool IsAlive() const;

	// ----------------------- ICollisionObjectの実装 ----------------------- //
	Vector3 GetCollisionPosition() const override;
	float   GetCollisionRadius() const override;
	CollisionLayer GetCollisionLayer() const override;
	void OnCollision(ICollisionObject* other) override;

private:
	Camera* camera_ = nullptr;
	DirectXCommon* dxCommon_ = nullptr;

	Object3d* object3d_ = nullptr;

	Transform transform_;

	float speed_ = 0.01f;
	Vector3 direction_;

	float radius_ = 0.08f;

	float lifeTime_ = 5.0f;  // 寿命(秒)
	float lifeTimer_ = 0.0f; // 経過時間(秒)
	bool  isAlive_ = true;   // 生存フラグ

	std::unique_ptr<ParticleEmitter> trailEmitter_ = nullptr;
};