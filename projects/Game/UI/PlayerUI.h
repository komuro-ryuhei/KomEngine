#pragma once

#include "Engine/Base/2d/Sprite/Sprite.h"
#include "Game/UI/PlayerHpUI.h"

#include <memory>

class PlayerUI {

public:

	void Init(int playerMaxHp);

	void Update(
		float dt,
		int playerHp,
		bool showRushRightClickGuide
	);

	void Draw(bool isClearUi);

	void ImGuiDebug();

private:

	void InitGuideUI();
	void InitRushRightClickGuide();

	void UpdateHp(int playerHp);
	void UpdateGuideUI();
	void UpdateRushRightClickGuide(float dt, bool shouldShowRushGuide);

	void DrawGuideUI();
	void DrawRushRightClickGuide();

private:

	// -----------------------
	// HP UI
	// -----------------------
	std::unique_ptr<PlayerHpUI> hpUI_ = nullptr;

	int playerMaxHp_ = 5;
	Vector2 hpStartPos_{ 20.0f, 700.0f };
	float hpHeartInterval_ = 52.0f;
	Vector2 hpHeartSize_{ 64.0f, 64.0f };

	// -----------------------
	// 操作ガイド UI
	// -----------------------
	std::unique_ptr<Sprite> controlGuideSprite_ = nullptr;
	std::unique_ptr<Sprite> controlGuideSprite2_ = nullptr;

	Vector2 controlGuide1Pos_{ 1120.0f, 480.0f };
	Vector2 controlGuide1Size_{ 256.0f, 256.0f };

	Vector2 controlGuide2Pos_{ 1120.0f, 480.0f };
	Vector2 controlGuide2Size_{ 220.0f, 240.0f };

	std::unique_ptr<Sprite> toPauseSpr_ = nullptr;
	Vector2 toPausePos_{ 1180.0f, 100.0f };
	Vector2 toPauseSize_{ 64.0f, 64.0f };

	// -----------------------
	// 突進スロー中の右クリックガイド
	// -----------------------
	std::unique_ptr<Sprite> rushRightClickGuide_ = nullptr;
	std::unique_ptr<Sprite> rushRightClickTogetoge_ = nullptr;

	bool rushRightClickGuideVisible_ = false;
	float rushRightClickGuideTimer_ = 0.0f;
	float rushRightClickGuideAlpha_ = 0.0f;

	Vector2 rushRightClickGuidePos_ = { 1100.0f, 220.0f };
	Vector2 rushRightClickGuideBaseSize_ = { 180.0f, 180.0f };

	float rushRightClickGuidePulseSpeed_ = 10.0f;
	float rushRightClickGuidePulseScale_ = 0.12f;
	float rushRightClickGuideBobAmp_ = 8.0f;
	float rushRightClickGuideFadeInSpeed_ = 8.0f;
	float rushRightClickGuideFadeOutSpeed_ = 10.0f;

	Vector2 rushRightClickTogetogeOffset_ = { 0.0f, 8.0f };
	Vector2 rushRightClickTogetogeBaseSize_ = { 230.0f, 230.0f };

	float rushRightClickTogetogePulseScale_ = 0.08f;
	float rushRightClickTogetogeRotateSpeed_ = 1.6f;
};