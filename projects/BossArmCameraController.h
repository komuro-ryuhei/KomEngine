#pragma once
#include "Engine/lib/Math/MyMath.h"

class Camera;
class BossEnemy;

/// 腕攻撃時の「カメラを腕の方向に少しだけ向ける」演出を管理する
class BossArmCameraController {

public:

    struct Params {
        float introTime = 0.15f; // 腕が伸びてる間にターゲット方向へ寄る時間
        float outroTime = 0.20f; // 腕が戻るときに元に戻る時間
        float lookWeight = 0.35f; // 腕方向にどれだけ寄せるか（0〜1）
        void ResetDefault() {
            introTime = 0.15f;
            outroTime = 0.20f;
            lookWeight = 0.35f;
        }
    };

    BossArmCameraController() = default;
    ~BossArmCameraController() = default;

    void Init(Camera* cam, BossEnemy* boss) {
        camera_ = cam;
        boss_ = boss;
        params_.ResetDefault();
        active_ = false;
        t_ = 0.0f;
    }

    // canControlCamera = 「今この攻撃演出でカメラを触っていいか？」（KO中・メテオ中などは false）
    void Update(float dt, bool canControlCamera);

    bool IsActive() const { return active_; }

    Params& GetParams() { return params_; }
    const Params& GetParams() const { return params_; }

private:

    void StartArmCamera();
    void UpdateIntro(float dt);
    void UpdateOutro(float dt);

private:

    Camera* camera_ = nullptr;
    BossEnemy* boss_ = nullptr;

    bool   active_ = false;
    float  t_ = 0.0f;   // 0→1 の補間用
    Vector3 savedRot_{};
    Vector3 targetRot_{};

    Params params_;
};