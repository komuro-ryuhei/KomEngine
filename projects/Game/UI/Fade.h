// Fade.h
#pragma once
#include <memory>
#include <algorithm>
#include <vector>
#include <random>
#include <cmath>
#include "Engine/Base/2d/Sprite/Sprite.h"

#include "Engine/Base/TextureManager/TextureManager.h"
#include "Engine/Base/System/System.h"

class Fade {
public:
    enum class Status { None, FadeIn, FadeOut };
    enum class Mode { kAlpha, kSlash, kDataError };

    void Initialize(int screenWidth, int screenHeight,
        const char* texturePath = "./Resources/images/uvChecker.png");

    void Start(Status status, float durationSec);
    void Stop();
    void Update();
    void Draw();
    bool IsFinished() const;

    void StartSlashOpen(float durationSec, float angleDeg = 45.0f, bool withFlash = true);

    // データエラー風
    void StartDataErrorClose(float durationSec); // Title → 暗転（FadeOut）
    void StartDataErrorOpen(float durationSec);  // Game  → 明転（FadeIn）

    // 既存
    static void SetDefaultOpenModeSlash(bool enabled);
    static bool GetDefaultOpenModeSlash();

    // 次シーンをDataErrorで開くためのフラグ
    static void SetDefaultOpenModeDataError(bool enabled);
    static bool GetDefaultOpenModeDataError();

private:
    static bool s_defaultSlashOpen;
    static bool s_defaultDataErrorOpen; // ★追加

    std::unique_ptr<Sprite> sprite_;

    Status status_ = Status::None;
    Mode   mode_ = Mode::kAlpha;

    float duration_ = 0.0f;
    float counter_ = 0.0f;
    int   w_ = 1280, h_ = 720;

    // Slash用（既存）
    std::unique_ptr<Sprite> sliceA_, sliceB_;
    std::unique_ptr<Sprite> flash_;
    float angleRad_ = 0.0f;
    bool  withFlash_ = true;
    float flashTime_ = 0.0f;
    const float flashLife_ = 0.12f;

    // ★DataError用
    std::unique_ptr<Sprite> rgbR_, rgbG_, rgbB_;
    std::unique_ptr<Sprite> scanline_;
    std::vector<std::unique_ptr<Sprite>> glitchStrips_;
    std::mt19937 rng_{ 12345u };
    float glitchHold_ = 0.0f;
};