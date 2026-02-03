#pragma once
#include <vector>
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
		AABB          box;   // AABB（CollisionTypes.h のやつ）
		CollisionLayer layer; // そのAABBが属しているレイヤー
	};

	// すべての登録オブジェクトのAABBを out に詰める
	void CollectDebugAABBs(std::vector<DebugAABBInfo>& out) const;

private:

	bool IsPairEnabled(CollisionLayer a, CollisionLayer b) const;

private:

	struct PairRule
	{
		CollisionLayer a;
		CollisionLayer b;
		bool enable;
	};

	std::vector<ICollisionObject*> objects_;
	std::vector<PairRule>          pairRules_;
};