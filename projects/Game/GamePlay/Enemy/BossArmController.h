#pragma once
#include "Engine/lib/Math/MyMath.h"
#include "externals/nlohmann/json.hpp"

class Camera;
class BossEnemy;

/// 腕攻撃時の「カメラを腕の方向に少しだけ向ける」演出を管理する
class BossArmController {

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

        // JSON から読み込む
        void LoadJSON(const nlohmann::json& j) {
            if (j.contains("introTime"))  introTime = j["introTime"];
            if (j.contains("outroTime"))  outroTime = j["outroTime"];
            if (j.contains("lookWeight")) lookWeight = j["lookWeight"];
        }

        // JSON へ書き出す
        void SaveJSON(nlohmann::json& j) const {

            auto R = [](float v) {
                return std::round(v * 1000.0f) / 1000.0f;
                };

            j["introTime"] = R(introTime);
            j["outroTime"] = R(outroTime);
            j["lookWeight"] = R(lookWeight);
        }
    };

    BossArmController() = default;
    ~BossArmController() = default;

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

public:

    // JSON 読み込み / 保存
    void LoadParamsFromJson(const std::string& path);
    void SaveParamsToJson(const std::string& path);

private:

    void StartArmCamera();
    void UpdateIntro(float dt);
    void UpdateOutro(float dt);

private:

    Camera* camera_ = nullptr;
    BossEnemy* boss_ = nullptr;

    bool   active_ = false;
    float  t_ = 0.0f; // 0→1 の補間用
    Vector3 savedRot_{};
    Vector3 targetRot_{};

    Params params_;
};