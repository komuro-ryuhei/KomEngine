// Fade.cpp
#include "Fade.h"

bool Fade::s_defaultSlashOpen = false;
bool Fade::s_defaultDataErrorOpen = false;

void Fade::SetDefaultOpenModeDataError(bool enabled) {
	s_defaultDataErrorOpen = enabled;
	if (enabled) s_defaultSlashOpen = false;
}
bool Fade::GetDefaultOpenModeDataError() { return s_defaultDataErrorOpen; }

void Fade::Initialize(int screenWidth, int screenHeight, const char* texturePath) {

	System::GetTextureManager()->LoadTexture("./Resources/images/white_1x1.png");

	w_ = screenWidth;
	h_ = screenHeight;

	sprite_ = std::make_unique<Sprite>();
	sprite_->Init("./Resources/images/white_1x1.png", BlendType::BLEND_ALPHA);
	sprite_->SetSize({ (float)w_, (float)h_ });
	sprite_->SetPosition({ 0.0f, 0.0f });
	sprite_->SetColor({ 0,0,0,1 }); // 黒に塗る

	// DataError用の生成
	rgbR_ = std::make_unique<Sprite>();
	rgbG_ = std::make_unique<Sprite>();
	rgbB_ = std::make_unique<Sprite>();
	rgbR_->Init("./Resources/images/white_1x1.png", BlendType::BLEND_ADD);
	rgbG_->Init("./Resources/images/white_1x1.png", BlendType::BLEND_ADD);
	rgbB_->Init("./Resources/images/white_1x1.png", BlendType::BLEND_ADD);
	rgbG_->SetSize({ (float)w_, (float)h_ });
	rgbB_->SetSize({ (float)w_, (float)h_ });

	scanline_ = std::make_unique<Sprite>();
	scanline_->Init("./Resources/images/white_1x1.png", BlendType::BLEND_ADD);
	scanline_->SetSize({ (float)w_, (float)h_ });

	const int kStrips = 24;
	glitchStrips_.resize(kStrips);
	for (int i = 0; i < kStrips; ++i) {
		glitchStrips_[i] = std::make_unique<Sprite>();
		glitchStrips_[i]->Init("./Resources/images/white_1x1.png", BlendType::BLEND_ADD);
		glitchStrips_[i]->SetSize({ (float)w_, 10.0f });
	}
}

void Fade::Start(Status status, float durationSec) {
	mode_ = Mode::kAlpha;
	status_ = status;
	duration_ = std::max(0.0f, durationSec);
	counter_ = 0.0f;

	if (!sprite_) return;

	// FadeOut: 0→1, FadeIn: 1→0
	if (status_ == Status::FadeOut) {
		sprite_->SetColor({ 0,0,0,0 });
	} else if (status_ == Status::FadeIn) {
		sprite_->SetColor({ 0,0,0,1 });
	}
}

void Fade::Stop() {
	status_ = Status::None;
}

bool Fade::IsFinished() const {
	if (status_ == Status::None) return true;
	if (duration_ <= 0.0f) return true;
	return counter_ >= duration_;
}

void Fade::StartDataErrorClose(float durationSec) {
	mode_ = Mode::kDataError;
	status_ = Status::FadeOut;
	duration_ = std::max(0.0f, durationSec);
	counter_ = 0.0f;
	glitchHold_ = 0.0f;
	sprite_->SetColor({ 0,0,0,0 });
}
void Fade::StartDataErrorOpen(float durationSec) {
	mode_ = Mode::kDataError;
	status_ = Status::FadeIn;
	duration_ = std::max(0.0f, durationSec);
	counter_ = 0.0f;
	glitchHold_ = 0.0f;
	sprite_->SetColor({ 0,0,0,1 });
}

void Fade::Update() {
	if (status_ == Status::None) return;

	counter_ += 1.0f / 60.0f;
	if (duration_ > 0.0f) counter_ = std::min(counter_, duration_);
	const float t = (duration_ > 0.0f) ? std::clamp(counter_ / duration_, 0.0f, 1.0f) : 1.0f;

	if (mode_ == Mode::kAlpha) {
		float a = (status_ == Status::FadeOut) ? t : (1.0f - t);
		sprite_->SetColor({ 0,0,0,a });
		sprite_->Update();
		return;
	}

	if (mode_ == Mode::kSlash) {

		const float cx = w_ * 0.5f, cy = h_ * 0.5f;
		const float diag = std::sqrt(float(w_) * w_ + float(h_) * h_);
		const float nx = std::cos(angleRad_), ny = std::sin(angleRad_);
		float off = diag * t;
		float dir = (angleRad_ >= 0.0f) ? 1.0f : -1.0f;

		sliceA_->SetPosition({ cx + nx * off * dir, cy + ny * off * dir });
		sliceB_->SetPosition({ cx - nx * off * dir, cy - ny * off * dir });
		sliceA_->Update();
		sliceB_->Update();

		if (withFlash_ && flash_) {
			flashTime_ += 1.0f / 60.0f;
			float a = std::max(0.0f, 1.0f - (flashTime_ / flashLife_));
			flash_->SetColor({ 1,1,1,a });
			flash_->Update();
		}
		return;
	}

	// DataError
	float baseA = (status_ == Status::FadeOut) ? t : (1.0f - t);
	sprite_->SetColor({ 0,0,0, baseA });
	sprite_->Update();

	// 断続的に強めグリッチ
	std::uniform_real_distribution<float> uf(0.0f, 1.0f);
	if (glitchHold_ > 0.0f) glitchHold_ -= 1.0f / 60.0f;
	if (glitchHold_ <= 0.0f) {
		float p = 0.08f + 0.55f * t;
		if (uf(rng_) < p) glitchHold_ = 0.04f + 0.10f * uf(rng_);
	}
	const float g = (glitchHold_ > 0.0f) ? 1.0f : 0.0f;
	float base = (status_ == Status::FadeOut) ? t : (1.0f - t); // FadeInはだんだん0へ

	const float intensity = std::clamp(0.10f + 0.90f * base, 0.0f, 1.0f) * (0.25f + 0.75f * g);

	// RGBズレ
	std::uniform_real_distribution<float> dxy(-10.0f, 10.0f);
	float offX = dxy(rng_) * intensity;
	float offY = dxy(rng_) * 0.25f * intensity;

	rgbR_->SetPosition({ 0.0f + offX, 0.0f + offY });
	rgbG_->SetPosition({ 0.0f - offX * 0.6f, 0.0f });
	rgbB_->SetPosition({ 0.0f + offX * 0.3f, 0.0f - offY });

	const float aR = 0.18f * intensity;
	const float aG = 0.18f * intensity;
	const float aB = 0.18f * intensity;

	rgbR_->SetColor({ 1.0f, 1.0f, 1.0f, aR });
	rgbG_->SetColor({ 1.0f, 1.0f, 1.0f, aG });
	rgbB_->SetColor({ 1.0f, 1.0f, 1.0f, aB });

	rgbR_->Update(); rgbG_->Update(); rgbB_->Update();

	// 走査線
	static float scanY = 0.0f;
	scanY += (30.0f + 180.0f * t) * (1.0f / 60.0f);
	if (scanY > (float)h_) scanY -= (float)h_;
	scanline_->SetPosition({ 0.0f, -scanY });
	scanline_->SetColor({ 1,1,1, 0.10f * intensity });
	scanline_->Update();

	// 横バグ帯
	std::uniform_real_distribution<float> uy(0.0f, (float)h_);
	std::uniform_real_distribution<float> uh(6.0f, 22.0f);
	std::uniform_real_distribution<float> bx(-120.0f, 120.0f);
	for (auto& s : glitchStrips_) {
		float y = uy(rng_);
		float h = uh(rng_);
		float x = bx(rng_) * intensity;
		s->SetSize({ (float)w_, h });
		s->SetPosition({ x, y });
		s->SetColor({ 1,1,1, 0.08f * intensity });
		s->Update();
	}
}

void Fade::Draw() {
	if (status_ == Status::None) return;

	if (mode_ == Mode::kAlpha) { sprite_->Draw(); return; }
	if (mode_ == Mode::kSlash) {
		sliceA_->Draw(); sliceB_->Draw();
		if (withFlash_ && flash_) flash_->Draw();
		return;
	}

	// DataError
	sprite_->Draw();
	rgbR_->Draw(); rgbG_->Draw(); rgbB_->Draw();
	scanline_->Draw();
	for (auto& s : glitchStrips_) s->Draw();
}

void Fade::SetDefaultOpenModeSlash(bool enabled) {
	s_defaultSlashOpen = enabled;
	if (enabled) s_defaultDataErrorOpen = false;
}

bool Fade::GetDefaultOpenModeSlash() {
	return s_defaultSlashOpen;
}

void Fade::StartSlashOpen(float durationSec, float angleDeg, bool withFlash) {
	mode_ = Mode::kSlash;
	status_ = Status::FadeIn;
	duration_ = std::max(0.0f, durationSec);
	counter_ = 0.0f;

	angleRad_ = angleDeg * 3.14159265f / 180.0f;
	withFlash_ = withFlash;
	flashTime_ = 0.0f;
}