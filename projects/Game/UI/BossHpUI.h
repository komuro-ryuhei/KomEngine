#pragma once

#include "Engine/Base/2d/Sprite/Sprite.h"

#include <memory>
#include <vector>

/// <summary>
/// ボスのHPを表示するUIクラス
/// </summary>
class BossHpUI {

public:

	void Init();

	void Update(int hp, int maxHp, float dt);

	void Draw();

	void OnHpChanged(int prevHp, int currentHp, int maxHp);

	void ImGuiDebug();

private:

	struct HpChip {
		std::unique_ptr<Sprite> sprite;
		Vector2 pos{};
		Vector2 vel{};
		float life = 0.0f;
	};

	// HPチップのスポーン
	void SpawnHpChips(float prevWidth, float newWidth);
	// HPバーの更新
	void UpdateHpBar(int hp, int maxHp);
	// HPチップの更新
	void UpdateHpChips(float dt);

private:

	// HPの中身
	std::unique_ptr<Sprite> hpSprite_ = nullptr;

	// HPの枠
	std::unique_ptr<Sprite> hpFrameSprite_ = nullptr;

	// ダメージ時に飛び散るHPチップ
	std::vector<HpChip> hpChips_;

	// HP中身
	Vector2 hpFillPosition_ = { 282.0f, 70.0f };
	Vector2 hpFillBaseSize_ = { 720.0f, 54.0f };

	// HP枠
	Vector2 hpFramePosition_ = { 165.0f, 70.0f };
	Vector2 hpFrameBaseSize_ = { 953.0f, 110.0f };

	float chipGravity_ = 900.0f;
};