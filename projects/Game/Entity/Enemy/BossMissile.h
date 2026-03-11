#pragma once
#include <memory>

#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Collision/ICollisionObject.h"

class Camera;
class Player;

class BossMissile : public ICollisionObject {

public:

    BossMissile() = default;
    ~BossMissile() override = default;

    void Init(Camera* camera);


    void Update(float dt);

    void Draw();

    void Spawn(const Vector3& startPos, const Vector3& direction, float speed);
    void Kill();
    bool IsAlive() const { return isAlive_; }
    bool DidHitPlayer() const { return hitPlayer_; }

    void SetTranslate(const Vector3& t) { transform_.translate = t; }
    const Vector3& GetTranslate() const { return transform_.translate; }

    // ICollisionObject
    Vector3 GetCollisionPosition() const override;
    float GetCollisionRadius() const override;
    CollisionLayer GetCollisionLayer() const override { return CollisionLayer::EnemyBullet; }
    void OnCollision(ICollisionObject* other) override;

private:
    Camera* camera_ = nullptr;
    std::unique_ptr<Object3d> object3d_ = nullptr;

    Transform transform_{};
    Vector3 direction_{ 0.0f, 0.0f, 1.0f };
    float speed_ = 0.0f;
    float radius_ = 0.7f;

    bool isAlive_ = false;
    bool hitPlayer_ = false;

    float lifeTimer_ = 0.0f;
    float maxLife_ = 6.0f;
};