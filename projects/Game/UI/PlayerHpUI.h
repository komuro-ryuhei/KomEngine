#pragma once

#include "Engine/Base/2d/Sprite/Sprite.h"

#include <memory>
#include <vector>

class PlayerHpUI {

public:

	void Init(int maxHp);

	void Update(int currentHp);

	void Draw();

	void SetLayout(const Vector2& startPos, float interval, const Vector2& heartSize);

private:

	void CreateHearts();

private:

	std::vector<std::unique_ptr<Sprite>> hpHearts_;

	int maxHp_ = 5;
	int currentHp_ = 5;

	Vector2 hpStartPos_{ 20.0f, 700.0f };
	float hpHeartInterval_ = 52.0f;
	Vector2 hpHeartSize_{ 64.0f, 64.0f };
};