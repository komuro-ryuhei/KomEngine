#pragma once

#include <cmath>
#include "externals/nlohmann/json.hpp"
#include "Engine/lib/Math/MyMath.h"

struct RushAttackParams {

	// 警告表示だけの時間
	float warningTime = 0.8f;

	// 溜め中に後ろへ引く移動時間
	float chargeMoveTime = 0.45f;

	// 突進にかかる時間
	float rushTime = 0.35f;

	// 元の位置へ戻る時間
	float returnTime = 0.75f;

	// 突進距離
	float rushDistance = 18.0f;

	// 溜め中に後ろへ引く距離
	float backAmount = 2.0f;

	// 溜め中の振動
	float chargeShakePower = 0.08f;
	float chargeShakeSpeed = 60.0f;

	// 警告スプライト
	Vector2 warningPos = { 640.0f, 160.0f };
	Vector2 warningSize = { 180.0f, 180.0f };
	float warningBlinkInterval = 0.12f;

	void LoadJSON(const nlohmann::json& j) {

		if (j.contains("warningTime")) warningTime = j["warningTime"];
		if (j.contains("chargeMoveTime")) chargeMoveTime = j["chargeMoveTime"];
		if (j.contains("rushTime")) rushTime = j["rushTime"];
		if (j.contains("returnTime")) returnTime = j["returnTime"];
		if (j.contains("rushDistance")) rushDistance = j["rushDistance"];
		if (j.contains("backAmount")) backAmount = j["backAmount"];
		if (j.contains("chargeShakePower")) chargeShakePower = j["chargeShakePower"];
		if (j.contains("chargeShakeSpeed")) chargeShakeSpeed = j["chargeShakeSpeed"];
		if (j.contains("warningBlinkInterval")) warningBlinkInterval = j["warningBlinkInterval"];

		if (j.contains("warningPos") && j["warningPos"].is_array()) {
			warningPos.x = j["warningPos"][0];
			warningPos.y = j["warningPos"][1];
		}

		if (j.contains("warningSize") && j["warningSize"].is_array()) {
			warningSize.x = j["warningSize"][0];
			warningSize.y = j["warningSize"][1];
		}
	}

	void SaveJSON(nlohmann::json& j) const {

		auto R = [](float v) {
			return std::round(v * 1000.0f) / 1000.0f;
			};

		j["warningTime"] = R(warningTime);
		j["chargeMoveTime"] = R(chargeMoveTime);
		j["rushTime"] = R(rushTime);
		j["returnTime"] = R(returnTime);
		j["rushDistance"] = R(rushDistance);
		j["backAmount"] = R(backAmount);
		j["chargeShakePower"] = R(chargeShakePower);
		j["chargeShakeSpeed"] = R(chargeShakeSpeed);
		j["warningBlinkInterval"] = R(warningBlinkInterval);
		j["warningPos"] = { R(warningPos.x), R(warningPos.y) };
		j["warningSize"] = { R(warningSize.x), R(warningSize.y) };
	}

	void ResetDefault() {
		warningTime = 0.8f;
		chargeMoveTime = 0.45f;
		rushTime = 0.35f;
		returnTime = 0.75f;
		rushDistance = 18.0f;
		backAmount = 2.0f;
		chargeShakePower = 0.08f;
		chargeShakeSpeed = 60.0f;
		warningPos = { 640.0f, 160.0f };
		warningSize = { 180.0f, 180.0f };
		warningBlinkInterval = 0.12f;
	}
};