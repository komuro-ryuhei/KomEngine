#include "BossArmCameraController.h"
#include "Engine/Base/Camera/Camera.h"
#include "Game/Entity/Enemy/BossEnemy.h"

void BossArmCameraController::StartArmCamera() {

    if (!camera_ || !boss_) return;

    active_ = true;
    t_ = 0.0f;

    // 元の向き保存
    savedRot_ = camera_->GetRotate();

    // 腕方向の目標回転を計算
    Vector3 camPos = camera_->GetTranaslate();
    Vector3 armPos = boss_->GetCurrentArmWorldPos();

    Vector3 to = MyMath::Normalize(armPos - camPos);
    float pitch = -std::asin(to.y);
    float yaw = std::atan2(to.x, to.z);

    Vector3 fullLook{ pitch, yaw, 0.0f };

    // 「ガッツリ向ける」んじゃなく、少しだけ腕方向を混ぜた角度を作る
    targetRot_ = MyMath::Lerp(savedRot_, fullLook, params_.lookWeight);
}

void BossArmCameraController::UpdateIntro(float dt) {

    if (!camera_) return;

    t_ += dt / params_.introTime;
    float t = MyMath::Clamp01(t_);
    camera_->SetRotate(MyMath::Lerp(savedRot_, targetRot_, t));
}

void BossArmCameraController::UpdateOutro(float dt) {

    if (!camera_) return;

    t_ += dt / params_.outroTime;
    float t = MyMath::Clamp01(t_);
    camera_->SetRotate(MyMath::Lerp(targetRot_, savedRot_, t));

    if (t >= 1.0f) {
        active_ = false;
        t_ = 0.0f;
    }
}

void BossArmCameraController::Update(float dt, bool canControlCamera) {

    if (!camera_ || !boss_) return;

    // カメラを触れない状態なら何もしない（状態も進めない）
    if (!canControlCamera) return;

    // まだアクティブじゃなくて、腕が伸び始めたら開始
    if (!active_ && boss_->IsExtending()) {
        StartArmCamera();
    }

    if (!active_) return;

    // 腕が伸びている間：ターゲット方向へ「じわっ」と向ける
    if (boss_->IsExtending()) {
        UpdateIntro(dt);
    }
    // 腕が戻り始めたら：元の向きへ戻す
    else {
        UpdateOutro(dt);
    }
}