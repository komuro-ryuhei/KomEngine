#include "CollisionManager.h"

void CollisionManager::Register(ICollisionObject* obj) {

	if (!obj) {
		return;
	}

	auto it = std::find(objects_.begin(), objects_.end(), obj);
	if (it == objects_.end()) {
		objects_.push_back(obj);
	}
}

void CollisionManager::Unregister(ICollisionObject* obj) {

	if (!obj) {
		return;
	}

	// 衝突ループ中は後回し
	if (isUpdating_) {
		auto it = std::find(pendingRemove_.begin(), pendingRemove_.end(), obj);
		if (it == pendingRemove_.end()) {
			pendingRemove_.push_back(obj);
		}
		return;
	}

	auto it = std::find(objects_.begin(), objects_.end(), obj);
	if (it != objects_.end()) {
		objects_.erase(it);
	}
}

void CollisionManager::AddPairRule(CollisionLayer a, CollisionLayer b) {

	PairRule r{ a, b, true };
	pairRules_.push_back(r);
}

void CollisionManager::Update() {

	isUpdating_ = true;

	const size_t n = objects_.size();
	for (size_t i = 0; i < n; ++i) {
		ICollisionObject* a = objects_[i];
		if (!a) {
			continue;
		}

		for (size_t j = i + 1; j < n; ++j) {
			ICollisionObject* b = objects_[j];
			if (!b) {
				continue;
			}

			if (!IsPairEnabled(a->GetCollisionLayer(), b->GetCollisionLayer())) {
				continue;
			}

			const float ra = a->GetCollisionRadius();
			const float rb = b->GetCollisionRadius();

			// 半径0以下は無効
			if (ra <= 0.0f || rb <= 0.0f) {
				continue;
			}

			AABB boxA = MakeAABBFromSphere(a->GetCollisionPosition(), ra);
			AABB boxB = MakeAABBFromSphere(b->GetCollisionPosition(), rb);

			if (IntersectAABB(boxA, boxB)) {
				a->OnCollision(b);
				b->OnCollision(a);
			}
		}
	}

	isUpdating_ = false;
	FlushPendingRemove();
}

void CollisionManager::FlushPendingRemove() {

	if (pendingRemove_.empty()) {
		return;
	}

	for (auto* obj : pendingRemove_) {
		auto it = std::find(objects_.begin(), objects_.end(), obj);
		if (it != objects_.end()) {
			objects_.erase(it);
		}
	}
	pendingRemove_.clear();
}

void CollisionManager::CollectDebugAABBs(std::vector<DebugAABBInfo>& out) const {

	out.clear();
	out.reserve(objects_.size());

	for (auto* obj : objects_) {
		if (!obj) {
			continue;
		}

		const float r = obj->GetCollisionRadius();
		if (r <= 0.0f) {
			continue;
		}

		DebugAABBInfo info;
		info.layer = obj->GetCollisionLayer();
		info.box = MakeAABBFromSphere(obj->GetCollisionPosition(), r);
		out.push_back(info);
	}
}

bool CollisionManager::IsPairEnabled(CollisionLayer a, CollisionLayer b) const {

	for (const auto& r : pairRules_) {
		if ((r.a == a && r.b == b) || (r.a == b && r.b == a)) {
			return r.enable;
		}
	}
	return false;
}