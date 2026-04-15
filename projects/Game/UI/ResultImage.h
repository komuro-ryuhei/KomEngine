#pragma once

#include "Engine/Base/2d/Sprite/Sprite.h"

class ResultImage {

public:

    void Init();

    void Update();

    void Draw();

    void StartSlideIn();
    bool IsSlideFinished() const { return state_ == State::Finished; }

private:

    enum class State {
        Idle,
        SlideIn,
        PopIn,
        Finished,
    };

private:

    std::unique_ptr<Sprite> blackBGSprite_ = nullptr;
    std::unique_ptr<Sprite> gameClearSprite_ = nullptr;
    std::unique_ptr<Sprite> pressKeySprite_ = nullptr;

    State state_ = State::Idle;

    float timer_ = 0.0f;

    float slideDuration_ = 0.28f;
    float popDuration_ = 0.35f;

    Vector2 blackStartPos_{ -720.0f, 360.0f };
    Vector2 blackEndPos_{ 640.0f, 360.0f };

    Vector2 clearBasePos_{ 640.0f, 180.0f };
    Vector2 clearBaseSize_{ 800.0f, 300.0f };

    Vector2 pressKeyPos_{ 640.0f, 520.0f };
    Vector2 pressKeySize_{ 420.0f, 64.0f };
};