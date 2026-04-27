#include "ParticleEditScene.h"

#include "Engine/Base/System/System.h"
#include "externals/imgui/imgui.h"

void ParticleEditScene::Init() {

	camera_ = std::make_unique<Camera>();
	camera_->Init();
	camera_->SetTranslate({ 0.0f, 3.0f, -12.0f });
	camera_->SetRotate({ 0.12f, 0.0f, 0.0f });

	skybox_ = std::make_unique<Skybox>();
	skybox_->Init("./Resources/images/blackCube.dds");

	debugLine_.Init(2048, BlendType::BLEND_ALPHA);
	debugLine_.SetCamera(camera_.get());

	particleEditor_.Init();
	particleEditor_.SetEnabled(true);
	particleEditor_.SetPreviewPosition(previewPos_);

	auto* pm = KomEngine::System::GetParticleManager();
if (pm) {
	pm->Init(BlendType::BLEND_ADD);
	pm->SetCamera(camera_.get());
}
}

void ParticleEditScene::Update() {

	const float dt = 1.0f / 60.0f;

	// 簡易カメラ移動
	Vector3 camPos = camera_->GetTranaslate();
	Vector3 camRot = camera_->GetRotate();

	if (KomEngine::System::PushKey(DIK_W)) { camPos.z += cameraMoveSpeed_; }
	if (KomEngine::System::PushKey(DIK_S)) { camPos.z -= cameraMoveSpeed_; }
	if (KomEngine::System::PushKey(DIK_A)) { camPos.x -= cameraMoveSpeed_; }
	if (KomEngine::System::PushKey(DIK_D)) { camPos.x += cameraMoveSpeed_; }
	if (KomEngine::System::PushKey(DIK_Q)) { camPos.y -= cameraMoveSpeed_; }
	if (KomEngine::System::PushKey(DIK_E)) { camPos.y += cameraMoveSpeed_; }

	if (KomEngine::System::PushKey(DIK_LEFT)) { camRot.y -= cameraRotSpeed_; }
	if (KomEngine::System::PushKey(DIK_RIGHT)) { camRot.y += cameraRotSpeed_; }
	if (KomEngine::System::PushKey(DIK_UP)) { camRot.x -= cameraRotSpeed_; }
	if (KomEngine::System::PushKey(DIK_DOWN)) { camRot.x += cameraRotSpeed_; }

	camera_->SetTranslate(camPos);
	camera_->SetRotate(camRot);
	camera_->Update();

	particleEditor_.SetEnabled(showEditor_);
	particleEditor_.SetPreviewPosition(previewPos_);
	particleEditor_.Update();

	KomEngine::System::GetParticleManager()->Update();

	debugLine_.Clear();
	if (showGrid_) {
		AddFloorGrid();
	}
	debugLine_.Update();

#ifdef USE_IMGUI
	ImGuiDebug();
#endif
}

void ParticleEditScene::Draw() {

	if (skybox_) {
		skybox_->Draw();
	}

	debugLine_.Draw();

	if (KomEngine::System::GetParticleManager()) {
		KomEngine::System::GetParticleManager()->Draw();
	}
}

void ParticleEditScene::Finalize() {
}

void ParticleEditScene::ImGuiDebug() {

	ImGui::Begin("Particle Edit Scene");

	ImGui::Checkbox("Show Editor", &showEditor_);
	ImGui::Checkbox("Show Grid", &showGrid_);

	ImGui::SeparatorText("Preview Position");
	ImGui::DragFloat3("Preview Pos", &previewPos_.x, 0.1f);

	if (ImGui::Button("Reset Preview Pos")) {
		previewPos_ = { 0.0f, 1.5f, 0.0f };
	}

	ImGui::SeparatorText("Camera");
	Vector3 camPos = camera_->GetTranaslate();
	Vector3 camRot = camera_->GetRotate();

	if (ImGui::DragFloat3("Camera Pos", &camPos.x, 0.1f)) {
		camera_->SetTranslate(camPos);
	}
	if (ImGui::DragFloat3("Camera Rot", &camRot.x, 0.01f)) {
		camera_->SetRotate(camRot);
	}

	if (ImGui::Button("Reset Camera")) {
		camera_->SetTranslate({ 0.0f, 3.0f, -12.0f });
		camera_->SetRotate({ 0.12f, 0.0f, 0.0f });
	}

	ImGui::Text("Move : W/A/S/D  Q/E");
	ImGui::Text("Rotate : Arrow Keys");

	ImGui::End();

	if (showEditor_) {
		particleEditor_.DrawImGui();
	}
}

void ParticleEditScene::AddFloorGrid() {

	const int halfCount = 20;
	const float spacing = 1.0f;
	const float y = 0.0f;
	const Vector4 color = { 0.2f, 0.45f, 1.0f, 1.0f };

	for (int i = -halfCount; i <= halfCount; ++i) {
		float x = static_cast<float>(i) * spacing;
		float z = static_cast<float>(i) * spacing;

		debugLine_.AddLine(
			{ x, y, -halfCount * spacing },
			{ x, y,  halfCount * spacing },
			color
		);

		debugLine_.AddLine(
			{ -halfCount * spacing, y, z },
			{  halfCount * spacing, y, z },
			color
		);
	}
}