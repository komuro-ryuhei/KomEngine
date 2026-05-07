#include "KnockoutCameraController.h"

void KnockoutCameraController::Start(Camera* cam, float groundY, const Params& p) {
    phase_ = Phase::Shock;
    t_ = 0.0f;
    startY_ = cam->GetTranaslate().y;
    groundY_ = groundY;
    savedRot_ = cam->GetRotate();
    params_ = p;
}

void KnockoutCameraController::Update(float dt, Camera* cam) {

    if (!IsActive()) return;
    t_ += dt;

    Vector3 pos = cam->GetTranaslate();
    Vector3 rot = cam->GetRotate();

    switch (phase_) {
    case Phase::Shock: {
        float a = std::clamp(t_ / params_.shockDur, 0.0f, 1.0f);
        // 瞬間ロール（衝撃）→ 0に戻す
        rot.z += params_.fallSide * MyMath::DegreeToRadian(3.0f) * (1.0f - a);
        cam->SetRotate(rot);
        if (t_ >= params_.shockDur) { phase_ = Phase::Knees; t_ = 0.0f; }
    } break;

    case Phase::Knees: {
        float a = EaseOutCubic(std::clamp(t_ / params_.kneesDur, 0.0f, 1.0f));
        pos.y = std::lerp(startY_, startY_ - params_.kneesDrop, a);
        rot.x = std::lerp(savedRot_.x, savedRot_.x + MyMath::DegreeToRadian(params_.kneesPitch), a);
        rot.z = std::lerp(savedRot_.z, savedRot_.z + params_.fallSide * MyMath::DegreeToRadian(params_.kneesRoll), a);
        cam->SetTranslate(pos); cam->SetRotate(rot);
        if (t_ >= params_.kneesDur) { phase_ = Phase::Fall; t_ = 0.0f; }
    } break;

    case Phase::Fall: {
        float a = EaseInCubic(std::clamp(t_ / params_.fallDur, 0.0f, 1.0f));
        float targetY = std::max(groundY_ + params_.floorGap, startY_ - params_.fallDrop);
        pos.y = std::lerp(pos.y, targetY, a);
        rot.x = std::lerp(rot.x, MyMath::DegreeToRadian(params_.fallPitch), a);
        rot.z = std::lerp(rot.z, params_.fallSide * MyMath::DegreeToRadian(params_.fallRoll), a);
        cam->SetTranslate(pos); cam->SetRotate(rot);
        if (t_ >= params_.fallDur) { phase_ = Phase::Land; t_ = 0.0f; }
    } break;

    case Phase::Land: {
        float a = std::clamp(t_ / params_.landDur, 0.0f, 1.0f);
        float bounce = std::sin(a * 3.141592f); // 0 → π
        rot.x = MyMath::DegreeToRadian(60.0f) + bounce * MyMath::DegreeToRadian(12.0f); // 軽いバウンド
        cam->SetRotate(rot);
        if (t_ >= params_.landDur) { phase_ = Phase::Blackout; t_ = 0.0f; }
    } break;

    case Phase::Blackout: {
        if (t_ >= params_.blackDur) { phase_ = Phase::Done; }
    } break;

    default: break;
    }
}