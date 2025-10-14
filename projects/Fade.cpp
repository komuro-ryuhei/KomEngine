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
    status_ = status;
    duration_ = std::max(0.0f, durationSec);
    counter_ = 0.0f;

    // 開始直後の見た目を明示
    if (status_ == Status::FadeOut) {
        sprite_->SetColor({ 0,0,0,0 });       // 透明→真っ黒へ
    } else if (status_ == Status::FadeIn) {
        sprite_->SetColor({ 0,0,0,1 });       // 真っ黒→透明へ
    }
}

void Fade::Stop() {
    status_ = Status::None;
}

void Fade::Update() {
    if (status_ == Status::None) return;

    // 固定フレーム前提（必要ならΔtに差し替え）
    counter_ += 1.0f / 60.0f;
    if (duration_ <= 0.0f) counter_ = 0.0f;                   // 即時
    if (duration_ > 0.0f)  counter_ = std::min(counter_, duration_);

    float t = (duration_ > 0.0f) ? std::clamp(counter_ / duration_, 0.0f, 1.0f) : 1.0f;

    float a = 0.0f;
    switch (status_) {
    case Status::FadeOut: a = t;        break;  // 0→1
    case Status::FadeIn:  a = 1.0f - t; break;  // 1→0
    default: break;
    }
    sprite_->SetColor({ 0.0f, 0.0f, 0.0f, a });

    sprite_->Update();
}

void Fade::Draw() {
    if (status_ == Status::None) return;      // 非アクティブ時は描画スキップ
    // ※あなたのエンジンはスプライトのPreDraw/PostDrawを外から呼ばなくても描けているので
    //   ここでは sprite_->Draw() のみにしています（最前面に出すため、各シーンのDraw末尾で呼ぶ）
    sprite_->Draw();
}

bool Fade::IsFinished() const {
    if (status_ == Status::None) return true;
    return counter_ >= duration_; // 1行でOK
}