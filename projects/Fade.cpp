#include "Fade.h"

void Fade::Initialize(int screenWidth, int screenHeight, const char* texturePath) {

    w_ = screenWidth;
    h_ = screenHeight;

    sprite_ = std::make_unique<Sprite>();
    // 乗算カラーで黒 (R,G,B=0)、アルファは更新で上書き
    sprite_->Init(texturePath, BlendType::BLEND_ALPHA);
    sprite_->SetSize({ static_cast<float>(w_), static_cast<float>(h_) });
    sprite_->SetPosition({ 0.0f, 0.0f });
    sprite_->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
}

void Fade::Start(Status status, float durationSec) {

    mode_ = Mode::kAlpha;
    status_ = status;
    duration_ = std::max(0.0f, durationSec);
    counter_ = 0.0f;

    // 開始直後の見た目を明示
    if (status_ == Status::FadeOut) {
        sprite_->SetColor({ 0,0,0,0 });
    } else if (status_ == Status::FadeIn) {
        sprite_->SetColor({ 0,0,0,1 });
    }
}

void Fade::Stop() {
    status_ = Status::None;
}

void Fade::Update() {

    if (status_ == Status::None) return;

    counter_ += 1.0f / 60.0f;
    if (duration_ > 0.0f) counter_ = std::min(counter_, duration_);
    const float t = (duration_ > 0.0f) ? std::clamp(counter_ / duration_, 0.0f, 1.0f) : 1.0f;

    if (mode_ == Mode::kAlpha) {
        // ノーマルフェード
        float a = (status_ == Status::FadeOut) ? t : (1.0f - t);
        sprite_->SetColor({ 0,0,0,a });
        sprite_->Update();
        return;
    }

    // --- 斜めスラッシュで“開く” ---
    const float cx = w_ * 0.5f, cy = h_ * 0.5f;
    const float diag = std::sqrt(float(w_) * w_ + float(h_) * h_);
    const float nx = std::cos(angleRad_), ny = std::sin(angleRad_);

    // イージング
    float off = diag * t;

    // ★角度が負(右上がり)なら向きを自動反転
    float dir = (angleRad_ >= 0.0f) ? 1.0f : -1.0f;

    sliceA_->SetPosition({ cx + nx * off * dir, cy + ny * off * dir });
    sliceB_->SetPosition({ cx - nx * off * dir, cy - ny * off * dir });

    sliceA_->Update();
    sliceB_->Update();

    // 斬撃の光（短時間で消える）
    if (withFlash_ && flash_) {
        flashTime_ += 1.0f / 60.0f;
        float a = std::max(0.0f, 1.0f - (flashTime_ / flashLife_));
        flash_->SetColor({ 1,1,1,a });
        flash_->Update();
    }
}

void Fade::Draw() {

    if (status_ == Status::None) return;

    if (mode_ == Mode::kAlpha) {
        sprite_->Draw();
        return;
    }

    // スラッシュ：黒2枚→最後に光
    sliceA_->Draw();
    sliceB_->Draw();
    if (withFlash_ && flash_) flash_->Draw();
}

bool Fade::IsFinished() const {

    if (status_ == Status::None) return true;
    return counter_ >= duration_; // 1行でOK
}

void Fade::StartSlashOpen(float durationSec, float angleDeg, bool withFlash) {

    mode_ = Mode::kSlash;
    status_ = Status::FadeIn;        // 「暗→明」に開く
    duration_ = std::max(0.0f, durationSec);
    counter_ = 0.0f;
    angleRad_ = angleDeg * 3.1415926535f / 180.0f;
    withFlash_ = withFlash;
    flashTime_ = 0.0f;

    // 2枚の黒スライスを生成
    if (!sliceA_) sliceA_ = std::make_unique<Sprite>();
    if (!sliceB_) sliceB_ = std::make_unique<Sprite>();
    sliceA_->Init("./Resources/images/uvChecker.png", BlendType::BLEND_ALPHA);
    sliceB_->Init("./Resources/images/uvChecker.png", BlendType::BLEND_ALPHA);

    // どの角度でも画面を覆えるようにでかい長方形に
    const float diag = std::sqrt(float(w_) * w_ + float(h_) * h_);
    const float cx = w_ * 0.5f, cy = h_ * 0.5f;

    // 片側だけ覆うために「左端アンカー」＋「切り口＝アンカー位置」にする
    sliceA_->SetAnchorPoint({ 1.0f, 0.5f });
    sliceB_->SetAnchorPoint({ 0.0f, 0.5f });
    sliceA_->SetSize({ diag * 2.0f, diag * 2.0f });
    sliceB_->SetSize({ diag * 2.0f, diag * 2.0f });

    sliceA_->SetRotation(angleDeg);
    sliceB_->SetRotation(angleDeg);

    // 切り口を画面中央にセット、黒で完全に覆う
    sliceA_->SetPosition({ cx, cy });
    sliceB_->SetPosition({ cx, cy });
    sliceA_->SetColor({ 0,0,0,1 });
    sliceB_->SetColor({ 0,0,0,1 });

    // 斬撃の光
    if (withFlash_) {
        if (!flash_) flash_ = std::make_unique<Sprite>();
        flash_->Init("./Resources/images/uvChecker.png", BlendType::BLEND_ADD);
        flash_->SetAnchorPoint({ 0.5f, 0.5f });
        flash_->SetRotation(angleRad_);
        flash_->SetPosition({ cx, cy });
        flash_->SetSize({ diag * 2.0f, 8.0f });
        flash_->SetColor({ 1,1,1,1 });
    }
}