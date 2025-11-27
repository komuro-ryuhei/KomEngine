// BossMeteorController.h
#pragma once
#include <memory>
#include <vector>

#include "Engine/lib/Math/MyMath.h"

class Camera;
class Player;
class BossEnemy;
class BossMeteor;

/// ボスのメテオ耐久フェーズ全体を制御するクラス
class BossMeteorController {
public:
    BossMeteorController() = default;
    ~BossMeteorController() = default;

    // 変数の初期化だけ行う
    void Init();

    // 依存オブジェクトをシーンから渡す
    void SetCamera(Camera* cam) { camera_ = cam; }
    void SetPlayer(Player* player) { player_ = player; }
    void SetBoss(BossEnemy* boss) { boss_ = boss; }
    void SetMeteors(std::vector<std::unique_ptr<BossMeteor>>* meteors) { meteors_ = meteors; }

    // メテオモード開始
    void Start();

    // 毎フレーム更新（メテオ中のみ呼ぶ or 常に呼んで中で分岐でもOK）
    void Update(float dt);

    // 強制終了（デバッグ用Mキーなど）
    void ForceEnd();

    // 状態確認
    bool IsActive() const { return phase_ != Phase::kIdle; }

private:
    enum class Phase { kIdle, kIntro, kShower, kOutro };

    void UpdateIntro(float dt);
    void UpdateShower(float dt);
    void UpdateOutro(float dt);

    void EndInternal(); // 実際の終了処理本体（ForceEnd/Outro両方から呼ぶ）

private:
    Camera* camera_ = nullptr;
    Player* player_ = nullptr;
    BossEnemy* boss_ = nullptr;
    std::vector<std::unique_ptr<BossMeteor>>* meteors_ = nullptr;

    // フェーズ
    Phase phase_ = Phase::kIdle;

    // カメラ保存＆補間
    Vector3 savedCamPos_{};
    Vector3 savedCamRot_{};
    Vector3 targetCamPosOffset_{ 0.0f, 2.0f, 0.0f }; // プレイヤー位置から少し上
    float   targetPitchUp_ = -0.45f;                 // 上向き(マイナスX回転)
    float   camLerp_ = 0.0f;
    float   camIntroTime_ = 0.6f;
    float   camOutroTime_ = 0.6f;

    // 進行管理
    float meteorModeTimer_ = 0.0f;
    float meteorModeDuration_ = 8.0f;    // 耐久時間

    // メテオスポーン
    float spawnInterval_ = 0.7f;
    float spawnTimer_ = 0.0f;
};