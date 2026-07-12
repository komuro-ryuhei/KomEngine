#include "CollisionManager.h"

size_t CollisionManager::ToIndex(CollisionLayer layer) const {

	return static_cast<size_t>(layer);
}

void CollisionManager::Register(ICollisionObject* obj) {

	if (!obj) {
		return;
	}

	// 全体リストに重複登録しない
	auto it = std::find(objects_.begin(), objects_.end(), obj);
	if (it != objects_.end()) {
		return;
	}

	objects_.push_back(obj);

	// レイヤー別リストにも登録
	const size_t layerIndex = ToIndex(obj->GetCollisionLayer());

	if (layerIndex >= objectsByLayer_.size()) {
		return;
	}

	objectsByLayer_[layerIndex].push_back(obj);
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

	RemoveObjectNow(obj);
}

void CollisionManager::AddPairRule(CollisionLayer a, CollisionLayer b) {

	// 同じ組み合わせがすでにあるなら enable だけ戻して終了
	for (auto& rule : pairRules_) {

		const bool sameOrder = (rule.a == a && rule.b == b);
		const bool reverseOrder = (rule.a == b && rule.b == a);

		if (sameOrder || reverseOrder) {
			rule.enable = true;
			return;
		}
	}

	PairRule r{ a, b, true };
	pairRules_.push_back(r);
}

void CollisionManager::Update() {

	isUpdating_ = true;

	// AddPairRule で登録されたレイヤー同士だけを判定する
	for (const auto& rule : pairRules_) {

		if (!rule.enable) {
			continue;
		}

		const size_t indexA = ToIndex(rule.a);
		const size_t indexB = ToIndex(rule.b);

		if (indexA >= objectsByLayer_.size() ||
			indexB >= objectsByLayer_.size()) {
			continue;
		}

		auto& listA = objectsByLayer_[indexA];
		auto& listB = objectsByLayer_[indexB];

		// 同じレイヤー同士の場合は、同じ組み合わせを2回見ないようにする
		if (indexA == indexB) {

			const size_t n = listA.size();

			for (size_t i = 0; i < n; ++i) {
				ICollisionObject* a = listA[i];
				if (!a) {
					continue;
				}

				for (size_t j = i + 1; j < n; ++j) {
					ICollisionObject* b = listA[j];
					if (!b) {
						continue;
					}

					CheckAndNotify(a, b);
				}
			}
		}
		else {

			for (ICollisionObject* a : listA) {
				if (!a) {
					continue;
				}

				for (ICollisionObject* b : listB) {
					if (!b) {
						continue;
					}

					CheckAndNotify(a, b);
				}
			}
		}
	}

	isUpdating_ = false;
	FlushPendingRemove();
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

void CollisionManager::CheckAndNotify(ICollisionObject* a, ICollisionObject* b) {

	if (!a || !b) {
		return;
	}

	// 念のため、ルール外なら判定しない
	if (!IsPairEnabled(a->GetCollisionLayer(), b->GetCollisionLayer())) {
		return;
	}

	const float ra = a->GetCollisionRadius();
	const float rb = b->GetCollisionRadius();

	// 半径0以下は無効
	if (ra <= 0.0f || rb <= 0.0f) {
		return;
	}

	AABB boxA = MakeAABBFromSphere(a->GetCollisionPosition(), ra);
	AABB boxB = MakeAABBFromSphere(b->GetCollisionPosition(), rb);

	if (IntersectAABB(boxA, boxB)) {
		a->OnCollision(b);
		b->OnCollision(a);
	}
}

void CollisionManager::RemoveFromList(
	std::vector<ICollisionObject*>& list,
	ICollisionObject* obj
) {

	auto it = std::remove(list.begin(), list.end(), obj);
	list.erase(it, list.end());
}

void CollisionManager::RemoveObjectNow(ICollisionObject* obj) {

	RemoveFromList(objects_, obj);

	const size_t layerIndex = ToIndex(obj->GetCollisionLayer());

	if (layerIndex < objectsByLayer_.size()) {
		RemoveFromList(objectsByLayer_[layerIndex], obj);
	}
	else {
		// 万が一レイヤー番号が範囲外なら全レイヤーから削除しておく
		for (auto& list : objectsByLayer_) {
			RemoveFromList(list, obj);
		}
	}
}

void CollisionManager::FlushPendingRemove() {

	if (pendingRemove_.empty()) {
		return;
	}

	for (auto* obj : pendingRemove_) {
		RemoveObjectNow(obj);
	}

	pendingRemove_.clear();
}