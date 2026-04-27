#pragma once
#include <string>
#include "Engine/Base/Particle/ParticleManager.h"

class ParticleEditor {

public:

	void Init();
	void Update();
	void DrawImGui();

	void SetEnabled(bool enabled) { isEnabled_ = enabled; }
	bool IsEnabled() const { return isEnabled_; }

	void SetPreviewPosition(const Vector3& pos) { previewPos_ = pos; }
	const Vector3& GetPreviewPosition() const { return previewPos_; }

private:

	bool isEnabled_ = true;
	bool isDirty_ = false;

	std::string selectedPresetName_;
	std::string saveFilePath_ = "./Resources/json/particles/test_particle.json";
	std::string loadFilePath_ = "./Resources/json/particles/test_particle.json";

	ParticlePreset editingPreset_{};

	uint32_t previewCount_ = 12;
	Vector3 previewPos_ = { 0.0f, 2.0f, 0.0f };

	bool autoPreviewEnabled_ = false;
	float autoPreviewInterval_ = 1.0f;
	float autoPreviewTimer_ = 0.0f;

private:

	void DrawPresetList();
	void DrawEditor();
	void DrawPreviewControls();

	void LoadFromManager(const std::string& name);
	void ApplyToManager();
	void CreateNewPreset();
};