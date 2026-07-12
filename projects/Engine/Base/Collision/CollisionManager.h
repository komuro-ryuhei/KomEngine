#pragma once
#include <vector>
#include <array>
#include <algorithm>

#include "ICollisionObject.h"
#include "CollisionTypes.h"

class CollisionManager {

public:

	// 登録・登録解除
	void Register(ICollisionObject* obj);
	void Unregister(ICollisionObject* obj);

	// 「どのレイヤー同士を判定するか」を設定
	void AddPairRule(CollisionLayer a, CollisionLayer b);

	// 毎フレーム呼ぶ
	void Update();

	// デバッグ用：各オブジェクトのAABB情報
	struct DebugAABBInfo
	{
		AABB box;
		CollisionLayer layer;
	};

	// すべての登録オブジェクトのAABBを out に詰める
	void CollectDebugAABBs(std::vector<DebugAABBInfo>& out) const;

private:

	struct PairRule
	{
		CollisionLayer a;
		CollisionLayer b;
		bool enable;
	};

private:

	static constexpr size_t kLayerCount =
		static_cast<size_t>(CollisionLayer::Environment) + 1;

private:

	size_t ToIndex(CollisionLayer layer) const;

	bool IsPairEnabled(CollisionLayer a, CollisionLayer b) const;

	void CheckAndNotify(ICollisionObject* a, ICollisionObject* b);

	void RemoveFromList(std::vector<ICollisionObject*>& list, ICollisionObject* obj);
	void RemoveObjectNow(ICollisionObject* obj);

	void FlushPendingRemove();

private:

	// 全オブジェクト一覧
	std::vector<ICollisionObject*> objects_;

	// レイヤー別のオブジェクト一覧
	std::array<std::vector<ICollisionObject*>, kLayerCount> objectsByLayer_;

	// 判定するレイヤーの組み合わせ
	std::vector<PairRule> pairRules_;

	bool isUpdating_ = false;
	std::vector<ICollisionObject*> pendingRemove_;
};