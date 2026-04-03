// BossMeteorController.h
#pragma once
#include <memory>
#include <vector>
#include "externals/nlohmann/json.hpp"

#include "Engine/lib/Math/MyMath.h"

class Camera;
class Player;
class BossEnemy;
class BossMeteor;

struct MeteorAttackParams {

	// デフォルト値
	float duration = 8.0f;
	float spawnInterval = 0.7f;
	float camIntroTime = 0.6f;
	float camOutroTime = 0.6f;
	Vector3 camOffset = { 0.0f, 2.0f, 0.0f };
	float pitchUp = -0.45f;

	// ================================
	// JSON から読み込む
	// ================================
	void LoadJSON(const nlohmann::json& j) {
		if (j.contains("duration"))      duration = j["duration"];
		if (j.contains("spawnInterval")) spawnInterval = j["spawnInterval"];
		if (j.contains("camIntroTime"))  camIntroTime = j["camIntroTime"];
		if (j.contains("camOutroTime"))  camOutroTime = j["camOutroTime"];

		if (j.contains("camOffset") && j["camOffset"].is_array()) {
			camOffset.x = j["camOffset"][0];
			camOffset.y = j["camOffset"][1];
			camOffset.z = j["camOffset"][2];
		}

		if (j.contains("pitchUp")) pitchUp = j["pitchUp"];
	}

	// ================================
	// JSON へ保存する
	// ================================
	void SaveJSON(nlohmann::json& j) const {

		auto R = [](float v) {
			return std::round(v * 1000.0f) / 1000.0f; // 小数3桁に丸め
			};

		j["duration"] = R(duration);
		j["spawnInterval"] = R(spawnInterval);
		j["camIntroTime"] = R(camIntroTime);
		j["camOutroTime"] = R(camOutroTime);
		j["camOffset"] = { R(camOffset.x), R(camOffset.y), R(camOffset.z) };
		j["pitchUp"] = R(pitchUp);
	}

	void ResetDefault() {
		duration = 8.0f;
		spawnInterval = 0.7f;
		camIntroTime = 0.6f;
		camOutroTime = 0.6f;
		camOffset = { 0.0f, 2.0f, 0.0f };
		pitchUp = -0.45f;
	}
};

/// ボスのメテオ耐久フェーズ全体を制御するクラス
class BossMeteorController {

public:
	BossMeteorController() = default;
	~BossMeteorController() = default;

	// 変数の初期化だけ行う
	void Init();

	// メテオモード開始
	void Start();

	// 毎フレーム更新（メテオ中のみ呼ぶ or 常に呼んで中で分岐でもOK）
	void Update(float dt);

	// 強制終了（デバッグ用Mキーなど）
	void ForceEnd();

	// 状態確認
	bool IsActive() const { return phase_ != Phase::kIdle; }

public:

	MeteorAttackParams& GetParams() { return params_; }
	const MeteorAttackParams& GetParams() const { return params_; }

	// 依存オブジェクトをシーンから渡す
	void SetCamera(Camera* cam) { camera_ = cam; }
	void SetPlayer(Player* player) { player_ = player; }
	void SetBoss(BossEnemy* boss) { boss_ = boss; }
	void SetMeteors(std::vector<std::unique_ptr<BossMeteor>>* meteors) { meteors_ = meteors; }

public:

	void LoadParamsFromJson(const std::string& path);

	void SaveParamsToJson(const std::string& path);

private:
	enum class Phase { kIdle, kIntro, kWaitClear, kShower, kOutro };

	void UpdateIntro(float dt);
	void UpdateWaitClear(float dt);
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
	float   camLerp_ = 0.0f;

	// 進行管理
	float meteorModeTimer_ = 0.0f;

	// メテオスポーン
	float spawnTimer_ = 0.0f;

	float enragedMeteorSpeedMul_ = 1.5f;     // 飛ぶ速度
	float enragedMeteorIntervalMul_ = 0.65f; // 出る間隔（小さいほど頻度UP）

	// Shower カメラ演出用
	bool    showerCamInited_ = false;
	float   showerCamT_ = 0.0f;
	Vector3 showerCamStartPos_{};
	Vector3 showerCamStartRot_{};

	// 待機時間
	float clearWaitTimer_ = 0.0f;
	float clearWaitDuration_ = 1.0f;

	// 
	float showerCamBlendTime_ = 0.6f;

	MeteorAttackParams params_;
};