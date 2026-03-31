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
	clearWaitTimer_ = 0.0f;
}

void BossMeteorController::Start() {

	if (!camera_ || !player_) return;

	if (boss_) {
		boss_->CancelAttacksForMeteor();
	}

	phase_ = Phase::kIntro;
	meteorModeTimer_ = 0.0f;
	spawnTimer_ = 0.0f;
	clearWaitTimer_ = 0.0f;

	savedCamPos_ = camera_->GetTranaslate();
	savedCamRot_ = camera_->GetRotate();
}

void BossMeteorController::Update(float dt) {

	if (phase_ == Phase::kIdle) return;
	if (!camera_ || !player_) return;

	switch (phase_) {
	case Phase::kIntro:     UpdateIntro(dt);     break;
	case Phase::kShower:    UpdateShower(dt);    break;
	case Phase::kWaitClear: UpdateWaitClear(dt); break;
	case Phase::kOutro:     UpdateOutro(dt);     break;
	case Phase::kIdle:
	default:
		break;
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

	// 既存ファイルがあれば読み込んでから上書き
	{
		std::ifstream ifs(path);
		if (!ifs.fail()) {
			try {
				ifs >> j;
			}
			catch (...) {
				j = nlohmann::json::object();
			}
		}
		else {
			j = nlohmann::json::object();
		}
	}

	// meteorAttack の JSON を構築
	nlohmann::json meteorJson;
	params_.SaveJSON(meteorJson);
	j["meteorAttack"] = meteorJson;

	std::ofstream ofs(path);
	ofs << std::setprecision(3) << j.dump(4); // 4はインデント
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
		showerCamInited_ = false;
		showerCamT_ = 0.0f;
	}
}

void BossMeteorController::UpdateShower(float dt) {

	if (!meteors_) return;

	Vector3 playerPos = player_->GetTransform().translate;

	meteorModeTimer_ += dt;
	spawnTimer_ += dt;

	// カメラ処理はそのまま
	{
		const Vector3 targetPos = playerPos + params_.camOffset;

		if (!showerCamInited_) {
			showerCamInited_ = true;
			showerCamT_ = 0.0f;
			showerCamStartPos_ = camera_->GetTranaslate();
			showerCamStartRot_ = camera_->GetRotate();
		}

		showerCamT_ += dt;
		float t = showerCamBlendTime_ > 0.0f ? (showerCamT_ / showerCamBlendTime_) : 1.0f;
		t = std::clamp(t, 0.0f, 1.0f);

		float ease = t * t * (3.0f - 2.0f * t);

		Vector3 targetRot = showerCamStartRot_;
		targetRot.x = params_.pitchUp;

		if (t < 1.0f) {
			camera_->SetTranslate(MyMath::Lerp(showerCamStartPos_, targetPos, ease));
			camera_->SetRotate(MyMath::Lerp(showerCamStartRot_, targetRot, ease));
		}
		else {
			const Vector3 curPos = camera_->GetTranaslate();
			const Vector3 curRot = camera_->GetRotate();

			const float posFollow = 1.0f - static_cast<float>(std::exp(-dt * 2.5f));
			const float rotFollow = 1.0f - static_cast<float>(std::exp(-dt * 3.0f));

			camera_->SetTranslate(MyMath::Lerp(curPos, targetPos, posFollow));

			Vector3 followTargetRot = curRot;
			followTargetRot.x = params_.pitchUp;
			camera_->SetRotate(MyMath::Lerp(curRot, followTargetRot, rotFollow));
		}
	}

	const bool enraged = (boss_ && boss_->IsEnraged());

	const float spawnInterval =
		enraged ? (params_.spawnInterval * enragedMeteorIntervalMul_)
		: params_.spawnInterval;

	const float speedMul =
		enraged ? enragedMeteorSpeedMul_
		: 1.0f;

	// duration 中だけ新規スポーン
	const bool canSpawn = (meteorModeTimer_ < params_.duration);

	if (canSpawn && spawnTimer_ >= spawnInterval) {
		spawnTimer_ = 0.0f;

		Vector3 camPos = camera_->GetTranaslate();
		Vector3 camRot = camera_->GetRotate();

		float cp = std::cos(camRot.x), sp = std::sin(camRot.x);
		float cy = std::cos(camRot.y), sy = std::sin(camRot.y);

		Vector3 forward = { sy * cp, -sp, cy * cp };
		Vector3 right = { cy, 0.0f, -sy };
		Vector3 up = { 0.0f, 1.0f, 0.0f };

		float dist = MyMath::Rand(110.0f, 160.0f);
		float spreadX = MyMath::Rand(-8.0f, 8.0f);
		float spreadUp = MyMath::Rand(8.0f, 18.0f);

		Vector3 start = camPos + forward * dist + right * spreadX + up * spreadUp;
		Vector3 target = camPos + forward * 6.0f + up * (-2.0f);

		float speed = (0.25f + 0.012f * dist) * speedMul;

		for (auto& m : *meteors_) {
			if (!m->IsAlive()) {
				m->SetScale({ 1.5f, 1.5f, 1.5f });
				m->SetGravity(0.0f);
				m->Spawn(start, target, speed);
				break;
			}
		}
	}

	// duration を過ぎたら待機フェーズへ
	if (!canSpawn) {
		phase_ = Phase::kWaitClear;
		clearWaitTimer_ = 0.0f;
	}
}

void BossMeteorController::UpdateWaitClear(float dt) {

	if (!meteors_) {
		phase_ = Phase::kOutro;
		meteorModeTimer_ = 0.0f;
		camLerp_ = 0.0f;
		return;
	}

	// この待機中もカメラは上向き維持
	Vector3 playerPos = player_->GetTransform().translate;
	Vector3 targetPos = playerPos + params_.camOffset;

	const Vector3 curPos = camera_->GetTranaslate();
	const Vector3 curRot = camera_->GetRotate();

	const float posFollow = 1.0f - static_cast<float>(std::exp(-dt * 2.5f));
	const float rotFollow = 1.0f - static_cast<float>(std::exp(-dt * 3.0f));

	camera_->SetTranslate(MyMath::Lerp(curPos, targetPos, posFollow));

	Vector3 targetRot = curRot;
	targetRot.x = params_.pitchUp;
	camera_->SetRotate(MyMath::Lerp(curRot, targetRot, rotFollow));

	int aliveMeteorCount = 0;
	for (const auto& m : *meteors_) {
		if (m && m->IsAlive()) {
			++aliveMeteorCount;
		}
	}

	// まだ残ってるなら待機タイマーは進めない
	if (aliveMeteorCount > 0) {
		clearWaitTimer_ = 0.0f;
		return;
	}

	// 全部消えたあとに2秒待つ
	clearWaitTimer_ += dt;
	if (clearWaitTimer_ >= clearWaitDuration_) {
		phase_ = Phase::kOutro;
		meteorModeTimer_ = 0.0f;
		camLerp_ = 0.0f;
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