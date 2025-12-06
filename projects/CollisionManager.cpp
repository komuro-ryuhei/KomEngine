#include "CollisionManager.h"

void CollisionManager::Register(ICollisionObject* obj) {

    // 
    objects_.push_back(obj);
}

void CollisionManager::Unregister(ICollisionObject* obj) {

    // 
    auto it = std::find(objects_.begin(), objects_.end(), obj);
    if (it != objects_.end()) {
        objects_.erase(it);
    }
}

// 「どのレイヤー同士を判定するか」を設定
void CollisionManager::AddPairRule(CollisionLayer a, CollisionLayer b) {

    // 
    PairRule r{ a, b, true };
    pairRules_.push_back(r);
}

// 毎フレーム呼ぶ
void CollisionManager::Update() {

    // 
    const size_t n = objects_.size();
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = i + 1; j < n; ++j)
        {
            ICollisionObject* a = objects_[i];
            ICollisionObject* b = objects_[j];
            if (!a || !b) continue;

            if (!IsPairEnabled(a->GetCollisionLayer(), b->GetCollisionLayer()))
                continue;

            // AABB 作成
            AABB boxA = MakeAABBFromSphere(
                a->GetCollisionPosition(),
                a->GetCollisionRadius()
            );
            AABB boxB = MakeAABBFromSphere(
                b->GetCollisionPosition(),
                b->GetCollisionRadius()
            );

            if (IntersectAABB(boxA, boxB))
            {
                // お互いに「当たったよ」と通知
                a->OnCollision(b);
                b->OnCollision(a);
            }
        }
    }
}

void CollisionManager::CollectDebugAABBs(std::vector<DebugAABBInfo>& out) const
{
    out.clear();
    out.reserve(objects_.size());

    for (auto* obj : objects_) {
        if (!obj) {
            continue;
        }

        DebugAABBInfo info;
        info.layer = obj->GetCollisionLayer();
        info.box = MakeAABBFromSphere(
            obj->GetCollisionPosition(),
            obj->GetCollisionRadius()
        );

        out.push_back(info);
    }
}

bool CollisionManager::IsPairEnabled(CollisionLayer a, CollisionLayer b) const {

    // 
    for (const auto& r : pairRules_) {
        if ((r.a == a && r.b == b) ||
            (r.a == b && r.b == a)) {
            return r.enable;
        }
    }
    return false;
}