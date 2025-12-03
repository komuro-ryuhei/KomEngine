// ICollisionObject.h
#pragma once
#include "CollisionTypes.h" // CollisionLayer, AABB など

class ICollisionObject
{
public:
    virtual ~ICollisionObject() = default;

    // 当たり判定の中心座標
    virtual Vector3 GetCollisionPosition() const = 0;

    // 当たり判定の大きさ（今は radius ベース）
    virtual float GetCollisionRadius() const = 0;

    // このオブジェクトのレイヤー（Player / Enemy / Bullet など）
    virtual CollisionLayer GetCollisionLayer() const = 0;

    // 何かに当たったとき呼ばれる
    virtual void OnCollision(ICollisionObject* other) = 0;
};