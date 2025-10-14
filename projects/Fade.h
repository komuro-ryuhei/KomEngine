#pragma once
#include <memory>
#include <algorithm>
#include "Engine/Base/2d/Sprite/Sprite.h"

class Fade {
public:
    enum class Status { None, FadeIn, FadeOut };

    // 画面サイズはウィンドウ解像度に合わせる
    void Initialize(int screenWidth, int screenHeight,
        const char* texturePath = "./Resources/images/uvChecker.png");

    // フェード開始（durationSec秒）
    void Start(Status status, float durationSec);

    // フェード停止（非表示＆計算停止）
    void Stop();

    void Update();
    void Draw();

    // 進行が完了したか（FadeIn / FadeOutとも）
    bool IsFinished() const;

    bool IsActive() const { return status_ != Status::None; }

private:
    std::unique_ptr<Sprite> sprite_;
    Status status_ = Status::None;
    float  duration_ = 0.0f;   // 継続時間[sec]
    float  counter_ = 0.0f;   // 経過時間[sec]
    int    w_ = 1280, h_ = 720;
};