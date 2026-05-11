// BossMeteorController.h
#pragma once
#include <memory>
#include <vector>
#include "externals/nlohmann/json.hpp"

#include "Engine/lib/Math/MyMath.h"
#include "Engine/Base/2d/Sprite/Sprite.h"
#include "Engine/Base/2d/Sprite/Sprite.h"

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

	void SaveJSON(nlohmann::json& j) const {

		auto R = [](float v) {
			return std::round(v * 1000.0f) / 1000.0f;
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

class BossMeteorController {

public:

	BossMeteorController();
	~BossMeteorController();

	void Init();

	void Update(float dt);

	void Draw();

	void Start();

	void ForceEnd();

	bool IsActive() const { return state_ != nullptr; }

public:

	// パラメータアクセス用
	MeteorAttackParams& GetParams() { return params_; }
	const MeteorAttackParams& GetParams() const { return params_; }

	void SetCamera(Camera* cam) { camera_ = cam; }
	void SetPlayer(Player* player) { player_ = player; }
	void SetBoss(BossEnemy* boss) { boss_ = boss; }
	void SetMeteors(std::vector<std::unique_ptr<BossMeteor>>* meteors) { meteors_ = meteors; }

public:

	// Jsonからのパラメータ読み込み・保存
	void LoadParamsFromJson(const std::string& path);
	void SaveParamsToJson(const std::string& path);

private:

	// メテオ攻撃専用の State Pattern インターフェイス
	class IMeteorPhaseState;
	class WarningState;
	class IntroState;
	class ShowerState;
	class WaitClearState;
	class OutroState;

	void ChangeState(std::unique_ptr<IMeteorPhaseState> nextState);
	bool IsWarningState() const;

	void UpdateWarning(float dt);
	void UpdateIntro(float dt);
	void UpdateWaitClear(float dt);
	void UpdateShower(float dt);
	void UpdateOutro(float dt);

	void EndInternal();

private:

	Camera* camera_ = nullptr;
	Player* player_ = nullptr;
	BossEnemy* boss_ = nullptr;
	std::vector<std::unique_ptr<BossMeteor>>* meteors_ = nullptr;

	std::unique_ptr<IMeteorPhaseState> state_;

	Vector3 savedCamPos_{};
	Vector3 savedCamRot_{};
	float   camLerp_ = 0.0f;

	float meteorModeTimer_ = 0.0f;
	float spawnTimer_ = 0.0f;

	float enragedMeteorSpeedMul_ = 1.5f;
	float enragedMeteorIntervalMul_ = 0.65f;

	bool    showerCamInited_ = false;
	float   showerCamT_ = 0.0f;
	Vector3 showerCamStartPos_{};
	Vector3 showerCamStartRot_{};

	float clearWaitTimer_ = 0.0f;
	float clearWaitDuration_ = 1.0f;

	float showerCamBlendTime_ = 0.6f;

	// =========================
	// 警告表示用
	// =========================
	std::unique_ptr<Sprite> warningSprite_ = nullptr;
	float warningTimer_ = 0.0f;
	float warningDuration_ = 2.0f;      // 2秒表示
	float warningBlinkInterval_ = 0.15f; // 点滅間隔
	bool  warningVisible_ = true;

	Vector2 warningPos_ = { 640.0f, 80.0f };
	Vector2 warningSize_ = { 220.0f, 220.0f };

	MeteorAttackParams params_;
};