#pragma once
#include "Engine/lib/Math/MyMath.h"  // Vector3 などが必要

// 衝突判定レイヤー
enum class CollisionLayer : uint8_t
{
    Player,
    Enemy,
    PlayerBullet,
    EnemyBullet,
    Environment,
    // 必要に応じて増やす
};

// AABB の構造体
struct AABB
{
    Vector3 min;
    Vector3 max;
};

// 中心＋半径 → AABB 生成（今のあなたの判定方式）
inline AABB MakeAABBFromSphere(const Vector3& center, float radius)
{
    Vector3 h{ radius, radius, radius };
    AABB box;
    box.min = center - h;
    box.max = center + h;
    return box;
}

// AABB 同士の交差判定
inline bool IntersectAABB(const AABB& a, const AABB& b)
{
    if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
    if (a.max.y < b.min.y || a.min.y > b.max.y) return false;
    if (a.max.z < b.min.z || a.min.z > b.max.z) return false;
    return true;
}