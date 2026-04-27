#include "ParticleEditor.h"
#include "Engine/Base/System/System.h"
#include "externals/imgui/imgui.h"
#include <cstring>

namespace {
	const char* BehaviorTypeItems[] = {
		"Default",
		"Explosion",
		"Hit",
		"Dust",
		"Ring",
		"MissileFlame",
		"ChargeCore",
		"ChargePulse",
		"PlayerChargeLine",
		"ChargeAura"
	};

	int BehaviorTypeToIndex(ParticleBehaviorType type) {
		switch (type) {
		case ParticleBehaviorType::Default:          return 0;
		case ParticleBehaviorType::Explosion:        return 1;
		case ParticleBehaviorType::Hit:              return 2;
		case ParticleBehaviorType::Dust:             return 3;
		case ParticleBehaviorType::Ring:             return 4;
		case ParticleBehaviorType::MissileFlame:     return 5;
		case ParticleBehaviorType::ChargeCore:       return 6;
		case ParticleBehaviorType::ChargePulse:      return 7;
		case ParticleBehaviorType::PlayerChargeLine: return 8;
		case ParticleBehaviorType::ChargeAura:       return 9;
		default:                                     return 0;
		}
	}

	ParticleBehaviorType IndexToBehaviorType(int index) {
		switch (index) {
		case 1: return ParticleBehaviorType::Explosion;
		case 2: return ParticleBehaviorType::Hit;
		case 3: return ParticleBehaviorType::Dust;
		case 4: return ParticleBehaviorType::Ring;
		case 5: return ParticleBehaviorType::MissileFlame;
		case 6: return ParticleBehaviorType::ChargeCore;
		case 7: return ParticleBehaviorType::ChargePulse;
		case 8: return ParticleBehaviorType::PlayerChargeLine;
		case 9: return ParticleBehaviorType::ChargeAura;
		default:return ParticleBehaviorType::Default;
		}
	}
}

void ParticleEditor::Init() {

	CreateNewPreset();
}

void ParticleEditor::Update() {

	if (!isEnabled_) {
		return;
	}
}

void ParticleEditor::DrawImGui() {

	if (!isEnabled_) {
		return;
	}

	ImGui::Begin("Particle Editor", &isEnabled_);

	DrawPresetList();
	ImGui::Separator();
	DrawEditor();
	ImGui::Separator();
	DrawPreviewControls();

	ImGui::End();
}

void ParticleEditor::DrawPresetList() {

	auto* pm = KomEngine::System::GetParticleManager();
	if (!pm) {
		ImGui::Text("ParticleManager not found.");
		return;
	}

	if (ImGui::Button("New Preset")) {
		CreateNewPreset();
	}

	ImGui::Spacing();
	ImGui::Text("Preset List");

	for (const auto& [name, preset] : pm->GetPresets()) {
		bool selected = (selectedPresetName_ == name);
		if (ImGui::Selectable(name.c_str(), selected)) {
			LoadFromManager(name);
		}
	}
}

void ParticleEditor::DrawEditor() {

	char nameBuf[128]{};
	char texBuf[256]{};
	char meshBuf[64]{};

	std::strncpy(nameBuf, editingPreset_.name.c_str(), sizeof(nameBuf) - 1);
	std::strncpy(texBuf, editingPreset_.textureFilePath.c_str(), sizeof(texBuf) - 1);
	std::strncpy(meshBuf, editingPreset_.meshType.c_str(), sizeof(meshBuf) - 1);

	ImGui::Text("Editing Preset");

	if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
		editingPreset_.name = nameBuf;
		isDirty_ = true;
	}
	if (ImGui::InputText("Texture Path", texBuf, sizeof(texBuf))) {
		editingPreset_.textureFilePath = texBuf;
		isDirty_ = true;
	}
	if (ImGui::InputText("Mesh Type", meshBuf, sizeof(meshBuf))) {
		editingPreset_.meshType = meshBuf;
		isDirty_ = true;
	}

	int behaviorIndex = BehaviorTypeToIndex(editingPreset_.behaviorType);
	if (ImGui::Combo("Behavior Type", &behaviorIndex, BehaviorTypeItems, IM_ARRAYSIZE(BehaviorTypeItems))) {
		editingPreset_.behaviorType = IndexToBehaviorType(behaviorIndex);
		isDirty_ = true;
	}

	ImGui::Checkbox("Billboard", &editingPreset_.billboard);

	ImGui::SeparatorText("LifeTime");
	if (ImGui::DragFloat2("lifeTime", &editingPreset_.lifeTime.min, 0.01f)) {
		isDirty_ = true;
	}

	ImGui::SeparatorText("Speed");
	if (ImGui::DragFloat2("speed", &editingPreset_.speed.min, 0.01f)) {
		isDirty_ = true;
	}

	ImGui::SeparatorText("Scale");
	if (ImGui::DragFloat2("scale", &editingPreset_.scale.min, 0.01f)) {
		isDirty_ = true;
	}

	ImGui::SeparatorText("Angle");
	if (ImGui::DragFloat2("angle", &editingPreset_.angle.min, 0.01f)) {
		isDirty_ = true;
	}

	ImGui::SeparatorText("VelocityY");
	if (ImGui::DragFloat2("velocityY", &editingPreset_.velocityY.min, 0.01f)) {
		isDirty_ = true;
	}

	ImGui::SeparatorText("Color Min");
	if (ImGui::ColorEdit4("colorMin", &editingPreset_.colorMin.x)) {
		isDirty_ = true;
	}

	ImGui::SeparatorText("Color Max");
	if (ImGui::ColorEdit4("colorMax", &editingPreset_.colorMax.x)) {
		isDirty_ = true;
	}

	if (ImGui::Button("Apply To Manager")) {
		ApplyToManager();
	}

	ImGui::SameLine();
	if (ImGui::Button("Reload Selected") && !selectedPresetName_.empty()) {
		LoadFromManager(selectedPresetName_);
	}

	ImGui::Text("Dirty: %s", isDirty_ ? "Yes" : "No");
}

void ParticleEditor::DrawPreviewControls() {

	auto* pm = KomEngine::System::GetParticleManager();
	if (!pm) {
		return;
	}

	char saveBuf[256]{};
	char loadBuf[256]{};
	std::strncpy(saveBuf, saveFilePath_.c_str(), sizeof(saveBuf) - 1);
	std::strncpy(loadBuf, loadFilePath_.c_str(), sizeof(loadBuf) - 1);

	ImGui::SeparatorText("Preview");

	ImGui::DragInt("Preview Count", reinterpret_cast<int*>(&previewCount_), 1, 1, 200);
	ImGui::DragFloat3("Preview Pos", &previewPos_.x, 0.1f);

	if (ImGui::Button("Preview Emit")) {
		if (!editingPreset_.name.empty()) {
			ApplyToManager();
			pm->Emit(editingPreset_.name, previewPos_, previewCount_);
		}
	}

	ImGui::SeparatorText("Save / Load");

	if (ImGui::InputText("Save Path", saveBuf, sizeof(saveBuf))) {
		saveFilePath_ = saveBuf;
	}
	if (ImGui::Button("Save Preset")) {
		ApplyToManager();
		pm->SavePresetToJson(editingPreset_.name, saveFilePath_);
	}

	if (ImGui::InputText("Load Path", loadBuf, sizeof(loadBuf))) {
		loadFilePath_ = loadBuf;
	}
	if (ImGui::Button("Load Preset")) {
		if (pm->LoadPresetFromJson(loadFilePath_)) {
			isDirty_ = false;
		}
	}
}

void ParticleEditor::LoadFromManager(const std::string& name) {

	auto* pm = KomEngine::System::GetParticleManager();
	if (!pm) {
		return;
	}

	const ParticlePreset* preset = pm->FindPreset(name);
	if (!preset) {
		return;
	}

	selectedPresetName_ = name;
	editingPreset_ = *preset;
	isDirty_ = false;
}

void ParticleEditor::ApplyToManager() {

	auto* pm = KomEngine::System::GetParticleManager();
	if (!pm) {
		return;
	}

	if (editingPreset_.name.empty()) {
		return;
	}

	pm->RegisterPreset(editingPreset_);
	pm->CreateParticleGroupFromPreset(editingPreset_.name);

	selectedPresetName_ = editingPreset_.name;
	isDirty_ = false;
}

void ParticleEditor::CreateNewPreset() {

	editingPreset_ = {};
	editingPreset_.name = "new_particle";
	editingPreset_.textureFilePath = "./Resources/images/circle2.png";
	editingPreset_.meshType = "a";
	editingPreset_.behaviorType = ParticleBehaviorType::Default;
	editingPreset_.lifeTime = { 0.30f, 0.60f };
	editingPreset_.speed = { 0.05f, 0.15f };
	editingPreset_.scale = { 0.10f, 0.25f };
	editingPreset_.angle = { 0.0f, 6.28318f };
	editingPreset_.velocityY = { 0.0f, 0.20f };
	editingPreset_.colorMin = { 1.0f, 1.0f, 1.0f, 0.6f };
	editingPreset_.colorMax = { 1.0f, 1.0f, 1.0f, 1.0f };
	editingPreset_.billboard = true;

	selectedPresetName_.clear();
	isDirty_ = true;
}