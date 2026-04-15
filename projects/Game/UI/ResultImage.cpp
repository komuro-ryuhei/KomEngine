#include "ResultImage.h"

#include "Engine/lib/Math/MyMath.h"
#include <algorithm>
#include <cmath>

void ResultImage::Init() {

	blackBGSprite_ = std::make_unique<Sprite>();
	blackBGSprite_->Init("./Resources/images/blackBG.png", BlendType::BLEND_ALPHA);
	blackBGSprite_->SetSize({ 1280.0f, 720.0f });
	blackBGSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	blackBGSprite_->SetPosition(blackStartPos_);
	blackBGSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

	gameClearSprite_ = std::make_unique<Sprite>();
	gameClearSprite_->Init("./Resources/images/gameClear.png", BlendType::BLEND_ALPHA);
	gameClearSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	gameClearSprite_->SetPosition(clearBasePos_);
	gameClearSprite_->SetSize({ 0.0f, 0.0f });
	gameClearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

	pressKeySprite_ = std::make_unique<Sprite>();
	pressKeySprite_->Init("./Resources/images/toTitle.png", BlendType::BLEND_ALPHA);
	pressKeySprite_->SetAnchorPoint({ 0.5f, 0.5f });
	pressKeySprite_->SetPosition(pressKeyPos_);
	pressKeySprite_->SetSize(pressKeySize_);
	pressKeySprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
}

void ResultImage::StartSlideIn() {

	state_ = State::SlideIn;
	timer_ = 0.0f;

	blackBGSprite_->SetPosition(blackStartPos_);
	blackBGSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

	gameClearSprite_->SetPosition(clearBasePos_);
	gameClearSprite_->SetSize({ 0.0f, 0.0f });
	gameClearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

	pressKeySprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
}

void ResultImage::Update() {

	constexpr float kDt = 1.0f / 60.0f;
	timer_ += kDt;

	if (state_ == State::SlideIn) {
		float t = std::clamp(timer_ / slideDuration_, 0.0f, 1.0f);
		float ease = t * t * (3.0f - 2.0f * t);

		Vector2 pos{
			MyMath::Lerp(blackStartPos_.x, blackEndPos_.x, ease),
			MyMath::Lerp(blackStartPos_.y, blackEndPos_.y, ease)
		};

		blackBGSprite_->SetPosition(pos);
		blackBGSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.82f });

		if (t >= 1.0f) {
			state_ = State::PopIn;
			timer_ = 0.0f;
		}
	}
	else if (state_ == State::PopIn) {
		float t = std::clamp(timer_ / popDuration_, 0.0f, 1.0f);

		float scale = 1.0f;
		if (t < 0.65f) {
			float u = t / 0.65f;
			scale = MyMath::Lerp(0.45f, 1.12f, u);
		}
		else {
			float u = (t - 0.65f) / 0.35f;
			scale = MyMath::Lerp(1.12f, 1.0f, std::clamp(u, 0.0f, 1.0f));
		}

		float alpha = std::min(1.0f, t * 1.8f);

		gameClearSprite_->SetPosition(clearBasePos_);
		gameClearSprite_->SetSize({
			clearBaseSize_.x * scale,
			clearBaseSize_.y * scale
			});
		gameClearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });

		if (t >= 1.0f) {
			state_ = State::Finished;
			timer_ = 0.0f;
		}
	}
	else if (state_ == State::Finished) {
		float blink = 0.55f + 0.45f * std::sin(timer_ * 4.0f);
		pressKeySprite_->SetColor({ 1.0f, 1.0f, 1.0f, blink });
	}

	blackBGSprite_->Update();
	gameClearSprite_->Update();
	pressKeySprite_->Update();
}

void ResultImage::Draw() {

	if (state_ == State::Idle) {
		return;
	}

	blackBGSprite_->Draw();

	if (state_ == State::PopIn || state_ == State::Finished) {
		gameClearSprite_->Draw();
	}

	if (state_ == State::Finished) {
		pressKeySprite_->Draw();
	}
}