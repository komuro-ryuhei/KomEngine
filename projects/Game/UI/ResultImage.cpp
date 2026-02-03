#include "ResultImage.h"

void ResultImage::Init() {

	// 
	blackBGSprite_ = std::make_unique<Sprite>();
	blackBGSprite_->Init("./Resources/images/blackBG.png", BlendType::BLEND_ALPHA);
	blackBGSprite_->SetSize({ 1280.0f,720.0f });
	blackBGSprite_->SetAnchorPoint({ 0.5f,0.5f });
	blackBGSprite_->SetPosition({ -720.0f,360.0f });
	blackBGSprite_->SetColor({ 1.0f,0.0f,0.0f,0.75f });

	gameClearSprite_ = std::make_unique<Sprite>();
	gameClearSprite_->Init("./Resources/images/gameClear.png", BlendType::BLEND_ALPHA);
	gameClearSprite_->SetSize({ 800.0f,300.0f });
	gameClearSprite_->SetAnchorPoint({ 0.5f,0.0f });
	gameClearSprite_->SetPosition({ 640.0f,200.0f });
}

void ResultImage::Update() {

	if (slideIn_) {
		slideTime_ += 1.0f / 60.0f;
		float t = std::clamp(slideTime_, 0.0f, 1.0f);

		float x = MyMath::Lerp(-360.0f, 640.0f, t);
		blackBGSprite_->SetPosition({ x, 360.0f });
	}

	blackBGSprite_->Update();
	gameClearSprite_->Update();
}

void ResultImage::Draw() {

	// 
	blackBGSprite_->Draw();

	if (IsSlideFinished()) {
		gameClearSprite_->Draw();
	}
}

void ResultImage::StartSlideIn() {
	slideIn_ = true;
	slideTime_ = 0.0f;
}