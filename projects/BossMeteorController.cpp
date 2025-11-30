// BossMeteorController.cpp
#include "BossMeteorController.h"

#include "Engine/Base/Camera/Camera.h"
#include "Game/Entity/Player/Player.h"
#include "Game/Entity/Enemy/BossEnemy.h"
#include "Game/Entity/Enemy/BossMeteor.h"
#include "Engine/lib/Math/MyMath.h"

void BossMeteorController::Init() {

    phase_ = Phase::kIdle;
    meteorModeTimer_ = 0.0f;
    spawnTimer_ = 0.0f;
    camLerp_ = 0.0f;
}

void BossMeteorController::Start() {

    if (!camera_ || !player_) return;

    phase_ = Phase::kIntro;
    meteorModeTimer_ = 0.0f;
    spawnTimer_ = 0.0f;

    // 現在のカメラ状態を保存（元 StartMeteorMode と同じ）
    savedCamPos_ = camera_->GetTranaslate();
    savedCamRot_ = camera_->GetRotate();
}

void BossMeteorController::Update(float dt) {

    if (phase_ == Phase::kIdle) return;
    if (!camera_ || !player_) return;

    switch (phase_) {
    case Phase::kIntro:  UpdateIntro(dt);  break;
    case Phase::kShower: UpdateShower(dt); break;
    case Phase::kOutro:  UpdateOutro(dt);  break;
    case Phase::kIdle:   default:          break;
    }
}

void BossMeteorController::ForceEnd() {

    if (phase_ == Phase::kIdle) return;
    EndInternal();
}

void BossMeteorController::LoadParamsFromJson(const std::string& path)
{
    std::ifstream file(path);
    if (file.fail()) {
        // ファイルが無ければデフォルトのまま開始
        return;
    }

    nlohmann::json j;
    file >> j;

    if (j.contains("meteorAttack")) {
        params_.LoadJSON(j["meteorAttack"]);
    }
}

void BossMeteorController::SaveParamsToJson(const std::string& path)
{
    nlohmann::json j;

    // meteorAttack の JSON を構築
    nlohmann::json meteorJson;
    params_.SaveJSON(meteorJson);
    j["meteorAttack"] = meteorJson;

    std::ofstream ofs(path);
    ofs << j.dump(4); // 4はインデント
}

void BossMeteorController::UpdateIntro(float dt) {

    Vector3 playerPos = player_->GetTransform().translate;

    meteorModeTimer_ += dt;
    camLerp_ = std::min(1.0f, meteorModeTimer_ / params_.camIntroTime);

    // 目標カメラ：プレイヤー位置 + 少し上、ピッチだけ上向きに
    Vector3 targetPos = playerPos + params_.camOffset;
    Vector3 targetRot = savedCamRot_;
    targetRot.x = params_.pitchUp;

    // 補間
    camera_->SetTranslate(MyMath::Lerp(savedCamPos_, targetPos, camLerp_));
    camera_->SetRotate(MyMath::Lerp(savedCamRot_, targetRot, camLerp_));

    if (camLerp_ >= 1.0f) {
        phase_ = Phase::kShower;
        meteorModeTimer_ = 0.0f;
    }
}

void BossMeteorController::UpdateShower(float dt) {

    if (!meteors_) return;

    Vector3 playerPos = player_->GetTransform().translate;

    meteorModeTimer_ += dt;
    spawnTimer_ += dt;

    // カメラはプレイヤー位置を追いながら“上向き固定”
    camera_->SetTranslate(playerPos + params_.camOffset);
    Vector3 rot = camera_->GetRotate();
    rot.x = params_.pitchUp;
    camera_->SetRotate(rot);

    // スポーン（元 UpdateMeteorMode の Shower 部分）
    if (spawnTimer_ >= params_.spawnInterval) {
        spawnTimer_ = 0.0f;

        // --- カメラ姿勢 ---
        Vector3 camPos = camera_->GetTranaslate();
        Vector3 camRot = camera_->GetRotate(); // rot.x = pitch, rot.y = yaw

        // --- カメラ基底ベクトル ---
        float cp = std::cos(camRot.x), sp = std::sin(camRot.x);
        float cy = std::cos(camRot.y), sy = std::sin(camRot.y);

        // 前方（rot=0 で +Z）
        Vector3 forward = { sy * cp, -sp, cy * cp };
        Vector3 right = { cy, 0.0f, -sy };
        Vector3 up = { 0.0f, 1.0f, 0.0f };

        // ---- 発生位置：前方かなり遠く（地平線付近）----
        float dist = MyMath::Rand(110.0f, 160.0f);
        float spreadX = MyMath::Rand(-8.0f, 8.0f);   // 左右
        float spreadUp = MyMath::Rand(8.0f, 18.0f);   // 少し上

        Vector3 start = camPos + forward * dist + right * spreadX + up * spreadUp;

        // ---- ターゲット：カメラの “すぐ手前” ----
        Vector3 target = camPos + forward * 6.0f + up * (-2.0f);

        // 距離に応じて速度を上げる（遠いほど速い）
        float speed = 0.25f + 0.012f * dist;

        // 空きスロットに生成
        for (auto& m : *meteors_) {
            if (!m->IsAlive()) {
                m->SetScale({ 1.5f, 1.5f, 1.5f });
                m->SetGravity(0.0f);
                m->Spawn(start, target, speed);
                break;
            }
        }
    }

    // 終了判定
    if (meteorModeTimer_ >= params_.duration) {
        phase_ = Phase::kOutro;
        meteorModeTimer_ = 0.0f;
    }
}

void BossMeteorController::UpdateOutro(float dt) {

    meteorModeTimer_ += dt;
    camLerp_ = std::min(1.0f, meteorModeTimer_ / params_.camOutroTime);

    // 目標は保存していた通常カメラ
    Vector3 curPos = camera_->GetTranaslate();
    Vector3 curRot = camera_->GetRotate();

    camera_->SetTranslate(MyMath::Lerp(curPos, savedCamPos_, camLerp_));
    camera_->SetRotate(MyMath::Lerp(curRot, savedCamRot_, camLerp_));

    if (camLerp_ >= 1.0f) {
        EndInternal();
    }
}

void BossMeteorController::EndInternal() {

    // 生きているメテオは爆発させる
    if (meteors_) {
        for (auto& m : *meteors_) {
            if (m->IsAlive()) m->Explode();
        }
    }

    // カメラを元に戻す
    if (camera_) {
        camera_->SetTranslate(savedCamPos_);
        camera_->SetRotate(savedCamRot_);
    }

    // ボスに「メテオ終わったよ」と伝える（元 EndMeteorMode と同じ）
    if (boss_) {
        boss_->OnMeteorFinished();
    }

    // フェーズリセット
    phase_ = Phase::kIdle;
    camLerp_ = 0.0f;
    meteorModeTimer_ = 0.0f;
    spawnTimer_ = 0.0f;
}