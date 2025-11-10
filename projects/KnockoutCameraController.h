#pragma once
#include "Engine/lib/Math/MyMath.h"
#include "Engine/Base/Camera/Camera.h"
#include "struct.h"
#include <algorithm>

/// <summary>
/// ゲームオーバー時の倒れこみ演出
/// </summary>
class KnockoutCameraController {

public:

    enum class Phase { None, Shock, Knees, Fall, Land, Blackout, Done };

    struct Params {
        // 時間（秒）
        float shockDur = 0.20f;
        float kneesDur = 0.80f;
        float fallDur = 0.60f;
        float landDur = 0.30f;
        float blackDur = 0.30f;
        // 角度（度）
        float kneesPitch = 8.0f; // 膝抜けの前傾
        float kneesRoll = 18.0f; // 膝抜けの傾き
        float fallPitch = 70.0f; // 倒れ込みの最終ピッチ
        float fallRoll = 38.0f;  // 倒れ込みの最終ロール
        // 高さ（m）
        float kneesDrop = 0.5f;  // 膝抜けで落ちる量
        float fallDrop = 1.1f;   // 合計で落ちる量（目線高）
        float floorGap = 0.04f;  // めり込み防止の浮かせ量
        // 倒れる方向（+1:右 / -1:左）
        int fallSide = +1;
    };

public:
    void Start(Camera* cam, float groundY, const Params& p);
    void Update(float dt, Camera* cam);

    bool  IsActive() const { return phase_ != Phase::None && phase_ != Phase::Done; }
    bool  IsDone()   const { return phase_ == Phase::Done; }
    Phase GetPhase() const { return phase_; }

    // 途中で方向だけ変えたいとき
    void  SetFallSide(int side) { params_.fallSide = (side >= 0) ? +1 : -1; }

private:
    static float EaseOutCubic(float x) { return 1.0f - std::pow(1.0f - x, 3.0f); }
    static float EaseInCubic(float x) { return x * x * x; }

private:
    Phase  phase_ = Phase::None;
    float  t_ = 0.0f;   // 現フェーズ経過
    float  startY_ = 0.0f;
    float  groundY_ = 0.0f;
    Vector3 savedRot_{};

    Params params_;
};