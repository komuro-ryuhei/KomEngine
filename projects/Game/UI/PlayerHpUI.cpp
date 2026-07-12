#include "PlayerHpUI.h"

#include <algorithm>

void PlayerHpUI::Init(int maxHp) {

	maxHp_ = std::max(1, maxHp);
	currentHp_ = maxHp_;

	CreateHearts();
}

void PlayerHpUI::Update(int currentHp) {

	currentHp_ = std::clamp(currentHp, 0, maxHp_);

	for (int i = 0; i < static_cast<int>(hpHearts_.size()); ++i) {

		auto& heart = hpHearts_[i];
		if (!heart) {
			continue;
		}

		heart->SetPosition({
			hpStartPos_.x + hpHeartInterval_ * i,
			hpStartPos_.y
			});

		heart->SetSize(hpHeartSize_);
		heart->Update();
	}
}

void PlayerHpUI::Draw() {

	const int drawCount = std::min(currentHp_, static_cast<int>(hpHearts_.size()));

	for (int i = 0; i < drawCount; ++i) {
		if (hpHearts_[i]) {
			hpHearts_[i]->Draw();
		}
	}
}

void PlayerHpUI::SetLayout(const Vector2& startPos, float interval, const Vector2& heartSize) {

	hpStartPos_ = startPos;
	hpHeartInterval_ = interval;
	hpHeartSize_ = heartSize;

	for (int i = 0; i < static_cast<int>(hpHearts_.size()); ++i) {
		if (!hpHearts_[i]) {
			continue;
		}

		hpHearts_[i]->SetPosition({
			hpStartPos_.x + hpHeartInterval_ * i,
			hpStartPos_.y
			});

		hpHearts_[i]->SetSize(hpHeartSize_);
		hpHearts_[i]->Update();
	}
}

void PlayerHpUI::CreateHearts() {

	hpHearts_.clear();
	hpHearts_.reserve(maxHp_);

	for (int i = 0; i < maxHp_; ++i) {

		auto heart = std::make_unique<Sprite>();
		heart->Init("./Resources/images/heart.png", BlendType::BLEND_ALPHA);
		heart->SetSize(hpHeartSize_);
		heart->SetAnchorPoint({ 0.0f, 1.0f });
		heart->SetPosition({
			hpStartPos_.x + hpHeartInterval_ * i,
			hpStartPos_.y
			});
		heart->Update();

		hpHearts_.push_back(std::move(heart));
	}
}