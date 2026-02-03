#pragma once
#include <memory>
#include "Engine/lib/Math/MyMath.h"

// 前方宣言
class Sprite;
class SceneManager;

class PauseMenu {

public:

    enum class Result {
        None,
        Resume,
        GoTitle,
    };

    PauseMenu() = default;
    ~PauseMenu() = default;

    void Init();
    void Toggle();
    void SetPaused(bool v) { isPaused_ = v; }
    bool IsPaused() const { return isPaused_; }

    // ポーズ中だけ呼ぶ：入力処理とカーソル制御
    Result Update(float dt);

    // ポーズ中だけ呼ぶ：UI描画（暗幕 + 項目）
    void Draw();

private:

    bool isPaused_ = false;
    int cursor_ = 0; // 0: Resume, 1: Title

    std::unique_ptr<Sprite> bg_;
    std::unique_ptr<Sprite> itemResume_;
    std::unique_ptr<Sprite> itemTitle_;
    std::unique_ptr<Sprite> cursorSpr_;
    std::unique_ptr<Sprite> pauseSpr_;

    Vector2 resumeBaseSize_ = { 320.0f, 64.0f };
    Vector2 titleBaseSize_ = { 320.0f, 64.0f };
    float hoverScale_ = 1.08f;
};