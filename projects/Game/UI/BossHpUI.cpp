#include "BossHpUI.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include "Engine/lib/Math/MyMath.h"

#include <algorithm>

void BossHpUI::Init() {

	// HPの中身
	hpSprite_ = std::make_unique<Sprite>();
	hpSprite_->Init("./Resources/images/hp.png", BlendType::BLEND_ALPHA);
	hpSprite_->SetAnchorPoint({ 0.0f, 0.5f });
	hpSprite_->SetSize(hpFillBaseSize_);
	hpSprite_->SetPosition(hpFillPosition_);
	hpSprite_->Update();

	// HPの枠
	hpFrameSprite_ = std::make_unique<Sprite>();
	hpFrameSprite_->Init("./Resources/images/bossHpFrame.png", BlendType::BLEND_ALPHA);
	hpFrameSprite_->SetAnchorPoint({ 0.0f, 0.5f });
	hpFrameSprite_->SetSize(hpFrameBaseSize_);
	hpFrameSprite_->SetPosition(hpFramePosition_);
	hpFrameSprite_->Update();

	hpChips_.clear();
}

void BossHpUI::Update(int hp, int maxHp, float dt) {

	UpdateHpBar(hp, maxHp);
	UpdateHpChips(dt);
}

void BossHpUI::Draw() {

	// HPの中身
	if (hpSprite_) {
		hpSprite_->Draw();
	}

	// 枠を上から描画
	if (hpFrameSprite_) {
		hpFrameSprite_->Draw();
	}

	// HPチップ
	for (auto& chip : hpChips_) {
		if (chip.sprite) {
			chip.sprite->Draw();
		}
	}
}

void BossHpUI::OnHpChanged(int prevHp, int currentHp, int maxHp) {

	if (maxHp <= 0) {
		return;
	}

	const float prevRatio = std::clamp(
		static_cast<float>(prevHp) / static_cast<float>(maxHp),
		0.0f,
		1.0f
	);

	const float currentRatio = std::clamp(
		static_cast<float>(currentHp) / static_cast<float>(maxHp),
		0.0f,
		1.0f
	);

	const float prevWidth = hpFillBaseSize_.x * prevRatio;
	const float currentWidth = hpFillBaseSize_.x * currentRatio;

	if (prevWidth > currentWidth) {
		SpawnHpChips(prevWidth, currentWidth);
	}
}

void BossHpUI::UpdateHpBar(int hp, int maxHp) {

	const float hpRatio =
		(maxHp > 0)
		? std::clamp(static_cast<float>(hp) / static_cast<float>(maxHp), 0.0f, 1.0f)
		: 0.0f;

	if (hpSprite_) {
		hpSprite_->SetPosition(hpFillPosition_);
		hpSprite_->SetSize({ hpFillBaseSize_.x * hpRatio, hpFillBaseSize_.y });
		hpSprite_->Update();
	}

	if (hpFrameSprite_) {
		hpFrameSprite_->SetPosition(hpFramePosition_);
		hpFrameSprite_->SetSize(hpFrameBaseSize_);
		hpFrameSprite_->Update();
	}
}

void BossHpUI::UpdateHpChips(float dt) {

	for (auto it = hpChips_.begin(); it != hpChips_.end();) {

		it->life -= dt;
		if (it->life <= 0.0f) {
			it = hpChips_.erase(it);
			continue;
		}

		// 重力
		it->vel.y += chipGravity_ * dt;

		// 位置更新
		it->pos.x += it->vel.x * dt;
		it->pos.y += it->vel.y * dt;

		if (it->sprite) {
			it->sprite->SetPosition(it->pos);
			it->sprite->Update();
		}

		++it;
	}
}

void BossHpUI::SpawnHpChips(float prevWidth, float newWidth) {

	if (!hpSprite_) {
		return;
	}

	const float lost = prevWidth - newWidth;
	if (lost <= 0.0f) {
		return;
	}

	// 減った幅に応じて個数を決める
	int count = static_cast<int>(lost / 25.0f) + 1;
	count = std::min(count, 30);

	const Vector2 basePos = hpFillPosition_;

	// 出現X範囲：減ったところ
	const float xMin = basePos.x + newWidth;
	const float xMax = basePos.x + prevWidth;

	for (int i = 0; i < count; ++i) {

		HpChip chip{};

		chip.sprite = std::make_unique<Sprite>();
		chip.sprite->Init("./Resources/images/hp.png", BlendType::BLEND_ALPHA);
		chip.sprite->SetAnchorPoint({ 0.5f, 0.5f });

		const float w = MyMath::Rand(6.0f, 12.0f);
		const float h = MyMath::Rand(6.0f, 12.0f);
		chip.sprite->SetSize({ w, h });

		// 緑色に着色
		chip.sprite->SetColor({ 0.2f, 1.0f, 0.2f, 1.0f });

		const float x = MyMath::Rand(xMin, xMax);
		const float y = basePos.y + MyMath::Rand(-4.0f, 4.0f);
		chip.pos = { x, y };

		chip.vel.x = MyMath::Rand(-120.0f, 120.0f);
		chip.vel.y = MyMath::Rand(-260.0f, -160.0f);

		chip.life = MyMath::Rand(0.5f, 0.9f);

		chip.sprite->SetPosition(chip.pos);
		chip.sprite->Update();

		hpChips_.push_back(std::move(chip));
	}
}

void BossHpUI::ImGuiDebug() {

#ifdef USE_IMGUI

	if (ImGui::Begin("Boss HP UI")) {

		ImGui::DragFloat2("Fill Pos", &hpFillPosition_.x, 1.0f);
		ImGui::DragFloat2("Fill Size", &hpFillBaseSize_.x, 1.0f, 1.0f, 2000.0f);

		ImGui::DragFloat2("Frame Pos", &hpFramePosition_.x, 1.0f);
		ImGui::DragFloat2("Frame Size", &hpFrameBaseSize_.x, 1.0f, 1.0f, 2000.0f);

		ImGui::DragFloat("Chip Gravity", &chipGravity_, 10.0f, 0.0f, 3000.0f);
		ImGui::Text("Chip Count: %d", static_cast<int>(hpChips_.size()));
	}
	ImGui::End();

#endif
}