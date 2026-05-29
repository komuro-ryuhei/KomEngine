#include "PauseMenu.h"

#include "Engine/Base/System/System.h"
#include "Engine/Base/2d/Sprite/Sprite.h"

#include <cmath>

void PauseMenu::Init() {

	// 暗幕
	bg_ = std::make_unique<Sprite>();
	bg_->Init("./Resources/images/blackBG.png", BlendType::BLEND_ALPHA);
	bg_->SetSize({ 1280.0f,720.0f });
	bg_->SetAnchorPoint({ 0.5f, 0.5f });
	bg_->SetPosition({ 640.0f, 360.0f });
	bg_->SetColor({ 1.0f,0.0f,0.0f,0.75f });

	// ポーズ画像
	pauseSpr_ = std::make_unique<Sprite>();
	pauseSpr_->Init("./Resources/images/pause.png", BlendType::BLEND_ALPHA);
	pauseSpr_->SetSize({ 320.0f, 64.0f });
	pauseSpr_->SetAnchorPoint({ 0.5f, 0.5f });
	pauseSpr_->SetPosition({ 640.0f, 180.0f });

	// 項目画像 
	itemResume_ = std::make_unique<Sprite>();
	itemResume_->Init("./Resources/images/returnGame.png", BlendType::BLEND_ALPHA);
	itemResume_->SetSize({ 480.0f, 192.0f });
	itemResume_->SetAnchorPoint({ 0.5f, 0.5f });
	itemResume_->SetPosition({ 640.0f, 320.0f });

	itemTitle_ = std::make_unique<Sprite>();
	itemTitle_->Init("./Resources/images/PauseToTitle.png", BlendType::BLEND_ALPHA);
	itemTitle_->SetSize({ 320.0f, 128.0f });
	itemTitle_->SetAnchorPoint({ 0.5f, 0.5f });
	itemTitle_->SetPosition({ 640.0f, 480.0f });
}

void PauseMenu::Toggle() { isPaused_ = !isPaused_; }

PauseMenu::Result PauseMenu::Update(float dt) {

	Input* input = KomEngine::System::GetInput();

	const POINT p = input->GetMousePosition();
	const Vector2 mouse{ (float)p.x, (float)p.y };

	const bool hoverResume = itemResume_ && itemResume_->HitTest(mouse);
	const bool hoverTitle = itemTitle_ && itemTitle_->HitTest(mouse);

	// ホバーで選択中の項目を決める
	if (hoverResume) {
		cursor_ = 0;
	} else if (hoverTitle) {
		cursor_ = 1;
	}

	// 選択中アニメーション時間更新
	selectedAnimTimer_ += dt;

	const float wave =
		std::sin(selectedAnimTimer_ * selectedScaleSpeed_) * selectedScaleAmplitude_;

	const float selectedScale = selectedBaseScale_ + wave;

	// ---- 選択中の項目だけふわふわ拡大縮小 ----
	if (itemResume_) {

		const float scale = (cursor_ == 0) ? selectedScale : 1.0f;

		itemResume_->SetSize({
			resumeBaseSize_.x * scale,
			resumeBaseSize_.y * scale
			});
	}

	if (itemTitle_) {

		const float scale = (cursor_ == 1) ? selectedScale : 1.0f;

		itemTitle_->SetSize({
			titleBaseSize_.x * scale,
			titleBaseSize_.y * scale
			});
	}

	// 左クリック決定
	if (input->TriggerMouse(0)) {
		if (hoverResume) {
			return Result::Resume;
		}

		if (hoverTitle) {
			return Result::GoTitle;
		}
	}

	// Escで閉じる
	if (input->TriggerKey(DIK_ESCAPE)) {
		return Result::Resume;
	}

	// 更新
	if (bg_) bg_->Update();
	if (pauseSpr_) pauseSpr_->Update();
	if (itemResume_) itemResume_->Update();
	if (itemTitle_) itemTitle_->Update();

	return Result::None;
}

void PauseMenu::Draw() {

	if (bg_) bg_->Draw();
	if (pauseSpr_) pauseSpr_->Draw();
	if (itemResume_) itemResume_->Draw();
	if (itemTitle_) itemTitle_->Draw();
	if (cursorSpr_) cursorSpr_->Draw();
}