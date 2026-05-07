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

	// 警告スプライト初期化
	warningSprite_ = std::make_unique<Sprite>();
	warningSprite_->Init("./Resources/images/exclamationMark.png", BlendType::BLEND_ALPHA);
	warningSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	warningSprite_->SetPosition(warningPos_);
	warningSprite_->SetSize(warningSize_);
	warningSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	warningSprite_->Update();

	warningTimer_ = 0.0f;
	warningVisible_ = true;
}

void BossMeteorController::Start() {

	if (!camera_ || !player_) return;

	if (boss_) {
		boss_->CancelAttacksForMeteor();
	}

	// まず警告フェーズへ
	phase_ = Phase::kWarning;

	meteorModeTimer_ = 0.0f;
	spawnTimer_ = 0.0f;
	clearWaitTimer_ = 0.0f;

	warningTimer_ = 0.0f;
	warningVisible_ = true;

	savedCamPos_ = camera_->GetTranaslate();
	savedCamRot_ = camera_->GetRotate();
}

void BossMeteorController::Update(float dt) {

	if (phase_ == Phase::kIdle) return;
	if (!camera_ || !player_) return;

	switch (phase_) {
	case Phase::kWarning:   UpdateWarning(dt);   break;
	case Phase::kIntro:     UpdateIntro(dt);     break;
	case Phase::kShower:    UpdateShower(dt);    break;
	case Phase::kWaitClear: UpdateWaitClear(dt); break;
	case Phase::kOutro:     UpdateOutro(dt);     break;
	case Phase::kIdle:
	default:
		break;
	}
}

void BossMeteorController::Draw() {

	if (phase_ == Phase::kWarning && warningSprite_ && warningVisible_) {
		warningSprite_->Draw();
	}
}

void BossMeteorController::UpdateWarning(float dt) {

	warningTimer_ += dt;

	// 点滅
	const int blinkIndex = static_cast<int>(warningTimer_ / warningBlinkInterval_);
	warningVisible_ = ((blinkIndex % 2) == 0);

	if (warningSprite_) {
		warningSprite_->SetPosition(warningPos_);
		warningSprite_->SetSize(warningSize_);
		warningSprite_->Update();
	}

	// 2秒経過後に本来のメテオ導入へ
	if (warningTimer_ >= warningDuration_) {
		warningVisible_ = false;
		phase_ = Phase::kIntro;
		meteorModeTimer_ = 0.0f;
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

	nlohmann::json meteorJson;
	params_.SaveJSON(meteorJson);
	j["meteorAttack"] = meteorJson;

	std::ofstream ofs(path);
	ofs << std::setprecision(3) << j.dump(4);
}

void BossMeteorController::UpdateIntro(float dt) {

	Vector3 playerPos = player_->GetTransform().translate;

	meteorModeTimer_ += dt;
	camLerp_ = std::min(1.0f, meteorModeTimer_ / params_.camIntroTime);

	Vector3 targetPos = playerPos + params_.camOffset;
	Vector3 targetRot = savedCamRot_;
	targetRot.x = params_.pitchUp;

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

	if (aliveMeteorCount > 0) {
		clearWaitTimer_ = 0.0f;
		return;
	}

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

	Vector3 curPos = camera_->GetTranaslate();
	Vector3 curRot = camera_->GetRotate();

	camera_->SetTranslate(MyMath::Lerp(curPos, savedCamPos_, camLerp_));
	camera_->SetRotate(MyMath::Lerp(curRot, savedCamRot_, camLerp_));

	if (camLerp_ >= 1.0f) {
		EndInternal();
	}
}

void BossMeteorController::EndInternal() {

	if (meteors_) {
		for (auto& m : *meteors_) {
			if (m->IsAlive()) m->Explode();
		}
	}

	if (camera_) {
		camera_->SetTranslate(savedCamPos_);
		camera_->SetRotate(savedCamRot_);
	}

	if (boss_) {
		boss_->OnMeteorFinished();
	}

	phase_ = Phase::kIdle;
	camLerp_ = 0.0f;
	meteorModeTimer_ = 0.0f;
	spawnTimer_ = 0.0f;

	warningTimer_ = 0.0f;
	warningVisible_ = false;
}