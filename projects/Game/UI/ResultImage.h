#pragma once

#include "Engine/Base/2d/Sprite/Sprite.h"

class ResultImage {

public:
    void Init();
    void Update();
    void Draw();

    // ★ スライド開始
    void StartSlideIn();

    bool IsSlideFinished() const { return slideIn_ && slideTime_ >= 1.0f; }

private:
    std::unique_ptr<Sprite> blackBGSprite_ = nullptr;
    std::unique_ptr<Sprite> gameClearSprite_ = nullptr;

    // ★ スライド用変数
    bool slideIn_ = false;
    float slideTime_ = 0.0f;  // 0 → 1 に進む
};
