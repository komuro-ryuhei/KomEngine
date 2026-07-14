#include "BossArmorController.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include "Engine/Base/System/System.h"
#include "Engine/Base/Particle/ParticleManager.h"

#include <algorithm>
#include <cmath>

namespace {
	const char* kBossArmorModel = "BossArmor.obj";
}

void BossArmorController::Init(Camera* camera, Object3d* parent) {

	camera_ = camera;
	parent_ = parent;

	Rebuild();
}

void BossArmorController::Update(float dt) {

	armorTime_ += dt;

	// 生きている装甲のインデックスを集める
	std::vector<int> aliveIdx;
	aliveIdx.reserve(armors_.size());

	for (int i = 0; i < static_cast<int>(armors_.size()); ++i) {
		if (armors_[i].obj && armors_[i].alive) {
			aliveIdx.push_back(i);
		}
	}

	const int aliveCount = static_cast<int>(aliveIdx.size());
	if (aliveCount <= 0) {
		return;
	}

	armorGlobalAngle_ += armorOrbitSpeed_ * dt;

	const float step = (MyMath::GetPI() * 2.0f) / static_cast<float>(aliveCount);

	for (int order = 0; order < aliveCount; ++order) {

		auto& armor = armors_[aliveIdx[order]];

		if (!armor.obj) {
			continue;
		}

		const float angle = armorGlobalAngle_ + step * static_cast<float>(order);

		float y = std::sinf(armorTime_ * armorFloatSpeed_ + angle) * armorFloatAmp_;

		Vector3 local{};
		local.x = std::cosf(angle) * armorOrbitRadius_;
		local.y = y;
		local.z = std::sinf(angle) * armorOrbitRadius_;

		armor.obj->SetTranslate(local);
		armor.obj->SetScale(armorScale_);
		armor.obj->Update();
	}
}

void BossArmorController::Draw() {

	for (auto& armor : armors_) {
		if (armor.obj && armor.alive) {
			armor.obj->Draw();
		}
	}
}

void BossArmorController::Reset(int hp) {

	if (hp <= 0) {
		hp = 1;
	}

	// 個数が変わっていたら作り直す
	if (static_cast<int>(armors_.size()) != armorInitialCount_) {
		Rebuild();
	}

	armorTime_ = 0.0f;
	armorGlobalAngle_ = 0.0f;

	const int count = static_cast<int>(armors_.size());

	for (int i = 0; i < count; ++i) {

		auto& armor = armors_[i];

		armor.alive = true;
		armor.hp = hp;

		float t = 0.0f;
		if (count > 0) {
			t = static_cast<float>(i) / static_cast<float>(count);
		}

		armor.angle = MyMath::GetPI() * 2.0f * t;

		if (armor.obj) {
			armor.obj->SetScale(armorScale_);
			armor.obj->SetParent(parent_);
		}
	}
}

bool BossArmorController::Damage(int damage, const Vector3& bossPos) {

	if (damage <= 0) {
		return false;
	}

	// 生きている装甲のうち、末尾側から1個選んでダメージ
	for (int i = static_cast<int>(armors_.size()) - 1; i >= 0; --i) {

		auto& armor = armors_[i];

		if (!armor.alive) {
			continue;
		}

		armor.hp -= damage;

		if (armor.hp <= 0) {

			armor.hp = 0;

			Vector3 breakPos = bossPos;

			if (armor.obj) {
				breakPos = armor.obj->GetWorldPosition();
			}

			armor.alive = false;

			auto* pm = KomEngine::System::GetParticleManager();
			if (pm) {
				if (pm->Exists("explosion")) {
					pm->Emit("explosion", breakPos, 12);
				}
				if (pm->Exists("hit")) {
					pm->Emit("hit", breakPos, 18);
				}
			}
		}

		return true;
	}

	return false;
}

bool BossArmorController::AreAllBroken() const {

	for (const auto& armor : armors_) {
		if (armor.alive) {
			return false;
		}
	}

	return true;
}

int BossArmorController::GetAliveCount() const {

	int count = 0;

	for (const auto& armor : armors_) {
		if (armor.alive) {
			++count;
		}
	}

	return count;
}

void BossArmorController::Rebuild() {

	armors_.clear();
	armors_.reserve(armorInitialCount_);

	armorTime_ = 0.0f;
	armorGlobalAngle_ = 0.0f;

	for (int i = 0; i < armorInitialCount_; ++i) {

		ArmorUnit armor{};

		armor.obj = std::make_unique<Object3d>();
		armor.obj->Init(BlendType::BLEND_NONE);
		armor.obj->SetModel(kBossArmorModel);
		armor.obj->SetDefaultCamera(camera_);
		armor.obj->SetParent(parent_);
		armor.obj->SetScale(armorScale_);

		float t = 0.0f;
		if (armorInitialCount_ > 0) {
			t = static_cast<float>(i) / static_cast<float>(armorInitialCount_);
		}

		armor.angle = MyMath::GetPI() * 2.0f * t;
		armor.alive = true;
		armor.hp = 3;

		armors_.push_back(std::move(armor));
	}
}

void BossArmorController::BreakOne() {

	if (GetAliveCount() <= 0) {
		return;
	}

	for (int i = static_cast<int>(armors_.size()) - 1; i >= 0; --i) {
		auto& armor = armors_[i];

		if (armor.alive) {
			armor.alive = false;
			return;
		}
	}
}

#ifdef USE_IMGUI
void BossArmorController::ImGuiDebug() {

	if (ImGui::Begin("Boss Armor")) {

		ImGui::Text("=== Armor Settings ===");

		ImGui::SliderInt("Armor Count", &armorInitialCount_, 1, 32);
		ImGui::SliderFloat("Orbit Radius", &armorOrbitRadius_, 0.0f, 20.0f);
		ImGui::SliderFloat("Orbit Speed", &armorOrbitSpeed_, -5.0f, 5.0f);
		ImGui::SliderFloat("Float Amp", &armorFloatAmp_, 0.0f, 5.0f);
		ImGui::SliderFloat("Float Speed", &armorFloatSpeed_, 0.0f, 10.0f);

		float scale[3] = {
			armorScale_.x,
			armorScale_.y,
			armorScale_.z
		};

		if (ImGui::DragFloat3("Armor Scale", scale, 0.01f, 0.01f, 5.0f)) {
			armorScale_.x = scale[0];
			armorScale_.y = scale[1];
			armorScale_.z = scale[2];

			for (auto& armor : armors_) {
				if (armor.obj) {
					armor.obj->SetScale(armorScale_);
				}
			}
		}

		if (ImGui::Button("Rebuild Armors")) {
			armorRebuildRequest_ = true;
		}

		ImGui::SameLine();

		if (ImGui::Button("Reset")) {
			armorInitialCount_ = 12;
			armorOrbitRadius_ = 4.8f;
			armorOrbitSpeed_ = 0.9f;
			armorFloatAmp_ = 0.18f;
			armorFloatSpeed_ = 1.6f;
			armorScale_ = { 0.7f, 0.7f, 0.7f };
			armorRebuildRequest_ = true;
		}
	}

	ImGui::End();

	if (armorRebuildRequest_) {
		Rebuild();
		armorRebuildRequest_ = false;
	}
}
#endif