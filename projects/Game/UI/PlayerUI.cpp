#include "PlayerUI.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include <cmath>
#include <algorithm>

void PlayerUI::Init(int playerMaxHp) {

	playerMaxHp_ = std::max(1, playerMaxHp);

	hpUI_ = std::make_unique<PlayerHpUI>();
	hpUI_->SetLayout(hpStartPos_, hpHeartInterval_, hpHeartSize_);
	hpUI_->Init(playerMaxHp_);

	InitGuideUI();
	InitRushRightClickGuide();
}

void PlayerUI::Update(
	float dt,
	int playerHp,
	bool showRushRightClickGuide
) {

	UpdateHp(playerHp);
	UpdateGuideUI();
	UpdateRushRightClickGuide(dt, showRushRightClickGuide);
}

void PlayerUI::Draw(bool isClearUi) {

	if (hpUI_) {
		hpUI_->Draw();
	}

	if (!isClearUi) {
		DrawGuideUI();
	}

	DrawRushRightClickGuide();
}

void PlayerUI::InitGuideUI() {

	controlGuideSprite_ = std::make_unique<Sprite>();
	controlGuideSprite_->Init("./Resources/images/mouseLeftClick.png", BlendType::BLEND_ALPHA);
	controlGuideSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	controlGuideSprite_->SetSize(controlGuide1Size_);
	controlGuideSprite_->SetPosition(controlGuide1Pos_);
	controlGuideSprite_->Update();

	controlGuideSprite2_ = std::make_unique<Sprite>();
	controlGuideSprite2_->Init("./Resources/images/mouseLightClickWithText.png", BlendType::BLEND_ALPHA);
	controlGuideSprite2_->SetAnchorPoint({ 0.5f, 0.5f });
	controlGuideSprite2_->SetSize(controlGuide2Size_);
	controlGuideSprite2_->SetPosition(controlGuide2Pos_);
	controlGuideSprite2_->Update();

	toPauseSpr_ = std::make_unique<Sprite>();
	toPauseSpr_->Init("./Resources/images/escape.png", BlendType::BLEND_ALPHA);
	toPauseSpr_->SetAnchorPoint({ 0.5f, 0.5f });
	toPauseSpr_->SetSize(toPauseSize_);
	toPauseSpr_->SetPosition(toPausePos_);
	toPauseSpr_->Update();
}

void PlayerUI::InitRushRightClickGuide() {

	// 背景のびっくり画像
	rushRightClickTogetoge_ = std::make_unique<Sprite>();
	rushRightClickTogetoge_->Init("./Resources/images/togetoge.png", BlendType::BLEND_ALPHA);
	rushRightClickTogetoge_->SetAnchorPoint({ 0.5f, 0.5f });
	rushRightClickTogetoge_->SetPosition({
		rushRightClickGuidePos_.x + rushRightClickTogetogeOffset_.x,
		rushRightClickGuidePos_.y + rushRightClickTogetogeOffset_.y
		});
	rushRightClickTogetoge_->SetSize(rushRightClickTogetogeBaseSize_);
	rushRightClickTogetoge_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	rushRightClickTogetoge_->Update();

	// 右クリック画像
	rushRightClickGuide_ = std::make_unique<Sprite>();
	rushRightClickGuide_->Init("./Resources/images/mouseRightClick.png", BlendType::BLEND_ALPHA);
	rushRightClickGuide_->SetAnchorPoint({ 0.5f, 0.5f });
	rushRightClickGuide_->SetPosition(rushRightClickGuidePos_);
	rushRightClickGuide_->SetSize(rushRightClickGuideBaseSize_);
	rushRightClickGuide_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	rushRightClickGuide_->Update();

	rushRightClickGuideVisible_ = false;
	rushRightClickGuideTimer_ = 0.0f;
	rushRightClickGuideAlpha_ = 0.0f;
}

void PlayerUI::UpdateHp(int playerHp) {

	if (!hpUI_) {
		return;
	}

	hpUI_->SetLayout(hpStartPos_, hpHeartInterval_, hpHeartSize_);
	hpUI_->Update(playerHp);
}

void PlayerUI::UpdateGuideUI() {

	if (controlGuideSprite_) {
		controlGuideSprite_->SetPosition(controlGuide1Pos_);
		controlGuideSprite_->SetSize(controlGuide1Size_);
		controlGuideSprite_->Update();
	}

	if (controlGuideSprite2_) {
		controlGuideSprite2_->SetPosition(controlGuide2Pos_);
		controlGuideSprite2_->SetSize(controlGuide2Size_);
		controlGuideSprite2_->Update();
	}

	if (toPauseSpr_) {
		toPauseSpr_->SetPosition(toPausePos_);
		toPauseSpr_->SetSize(toPauseSize_);
		toPauseSpr_->Update();
	}
}

void PlayerUI::UpdateRushRightClickGuide(float dt, bool shouldShowRushGuide) {

	if (!rushRightClickGuide_) {
		return;
	}

	rushRightClickGuideVisible_ = shouldShowRushGuide;

	if (rushRightClickGuideVisible_) {
		rushRightClickGuideTimer_ += dt;

		rushRightClickGuideAlpha_ += dt * rushRightClickGuideFadeInSpeed_;
		if (rushRightClickGuideAlpha_ > 1.0f) {
			rushRightClickGuideAlpha_ = 1.0f;
		}
	}
	else {
		rushRightClickGuideAlpha_ -= dt * rushRightClickGuideFadeOutSpeed_;
		if (rushRightClickGuideAlpha_ < 0.0f) {
			rushRightClickGuideAlpha_ = 0.0f;
			rushRightClickGuideTimer_ = 0.0f;
		}
	}

	// 表示していないなら透明のまま更新
	if (rushRightClickGuideAlpha_ <= 0.0f) {

		rushRightClickGuide_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
		rushRightClickGuide_->Update();

		if (rushRightClickTogetoge_) {
			rushRightClickTogetoge_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
			rushRightClickTogetoge_->Update();
		}

		return;
	}

	// ----------------------------
	// 右クリック画像
	// ----------------------------
	const float pulse =
		1.0f +
		std::sin(rushRightClickGuideTimer_ * rushRightClickGuidePulseSpeed_) *
		rushRightClickGuidePulseScale_;

	const float bobY =
		std::sin(rushRightClickGuideTimer_ * 6.0f) *
		rushRightClickGuideBobAmp_;

	const float flash =
		0.85f +
		0.15f *
		(0.5f + 0.5f * std::sin(rushRightClickGuideTimer_ * 14.0f));

	Vector2 guideSize = {
		rushRightClickGuideBaseSize_.x * pulse,
		rushRightClickGuideBaseSize_.y * pulse
	};

	Vector2 guidePos = rushRightClickGuidePos_;
	guidePos.y += bobY;

	rushRightClickGuide_->SetPosition(guidePos);
	rushRightClickGuide_->SetSize(guideSize);
	rushRightClickGuide_->SetColor({
		1.0f,
		1.0f,
		1.0f,
		rushRightClickGuideAlpha_ * flash
		});
	rushRightClickGuide_->Update();

	// ----------------------------
	// 背景の tog etoge
	// ----------------------------
	if (rushRightClickTogetoge_) {

		const float togPulse =
			1.0f +
			std::sin(rushRightClickGuideTimer_ * 8.0f) *
			rushRightClickTogetogePulseScale_;

		Vector2 togPos = {
			guidePos.x + rushRightClickTogetogeOffset_.x,
			guidePos.y + rushRightClickTogetogeOffset_.y
		};

		Vector2 togSize = {
			rushRightClickTogetogeBaseSize_.x * togPulse,
			rushRightClickTogetogeBaseSize_.y * togPulse
		};

		const float togRot =
			std::sin(rushRightClickGuideTimer_ * rushRightClickTogetogeRotateSpeed_) *
			0.10f;

		rushRightClickTogetoge_->SetPosition(togPos);
		rushRightClickTogetoge_->SetSize(togSize);
		rushRightClickTogetoge_->SetRotation(togRot);
		rushRightClickTogetoge_->SetColor({
			1.0f,
			1.0f,
			1.0f,
			rushRightClickGuideAlpha_ * 0.9f
			});
		rushRightClickTogetoge_->Update();
	}
}

void PlayerUI::DrawGuideUI() {

	if (controlGuideSprite_) {
		controlGuideSprite_->Draw();
	}

	// 今はDrawしていなかったので、必要になったら有効化
	// if (controlGuideSprite2_) {
	// 	controlGuideSprite2_->Draw();
	// }

	if (toPauseSpr_) {
		toPauseSpr_->Draw();
	}
}

void PlayerUI::DrawRushRightClickGuide() {

	if (rushRightClickGuideAlpha_ <= 0.0f) {
		return;
	}

	// 下地のびっくり画像
	if (rushRightClickTogetoge_) {
		rushRightClickTogetoge_->Draw();
	}

	// 右クリック画像
	if (rushRightClickGuide_) {
		rushRightClickGuide_->Draw();
	}
}

void PlayerUI::ImGuiDebug() {

#ifdef USE_IMGUI

	if (controlGuideSprite_) {
		controlGuideSprite_->ImGuiDebug();
	}

	if (controlGuideSprite2_) {
		controlGuideSprite2_->ImGuiDebug();
	}

	ImGui::Separator();
	ImGui::Text("Player UI Layout");

	ImGui::DragFloat2("HP Pos", &hpStartPos_.x, 1.0f);
	ImGui::DragFloat("HP Interval", &hpHeartInterval_, 1.0f, 1.0f, 300.0f);
	ImGui::DragFloat2("HP Size", &hpHeartSize_.x, 1.0f, 1.0f, 300.0f);

	ImGui::Separator();
	ImGui::Text("Guide UI Layout");

	ImGui::DragFloat2("Guide1 Pos", &controlGuide1Pos_.x, 1.0f);
	ImGui::DragFloat2("Guide1 Size", &controlGuide1Size_.x, 1.0f, 1.0f, 2000.0f);

	ImGui::DragFloat2("Guide2 Pos", &controlGuide2Pos_.x, 1.0f);
	ImGui::DragFloat2("Guide2 Size", &controlGuide2Size_.x, 1.0f, 1.0f, 2000.0f);

	ImGui::DragFloat2("Pause Pos", &toPausePos_.x, 1.0f);
	ImGui::DragFloat2("Pause Size", &toPauseSize_.x, 1.0f, 1.0f, 2000.0f);

	ImGui::Separator();
	ImGui::Text("Rush Right Click Guide");

	ImGui::DragFloat2("Rush Guide Pos", &rushRightClickGuidePos_.x, 1.0f);
	ImGui::DragFloat2("Rush Guide Size", &rushRightClickGuideBaseSize_.x, 1.0f, 1.0f, 1000.0f);

	ImGui::DragFloat2("Togetoge Offset", &rushRightClickTogetogeOffset_.x, 1.0f);
	ImGui::DragFloat2("Togetoge Size", &rushRightClickTogetogeBaseSize_.x, 1.0f, 1.0f, 1000.0f);

	ImGui::DragFloat("Guide Pulse Speed", &rushRightClickGuidePulseSpeed_, 0.1f, 0.0f, 50.0f);
	ImGui::DragFloat("Guide Pulse Scale", &rushRightClickGuidePulseScale_, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Guide Bob Amp", &rushRightClickGuideBobAmp_, 0.1f, 0.0f, 100.0f);

#endif
}