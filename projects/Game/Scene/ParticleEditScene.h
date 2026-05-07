#pragma once

#include "Game/Scene/IScene.h"
#include "Game/Scene/SceneManager.h"

#include "Engine/Base/Particle/ParticleEditor.h"
#include "Engine/Base/Debug/LineRenderer.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/3d/Skybox/Skybox.h"

class ParticleEditScene : public IScene {

public:

	ParticleEditScene() = default;
	~ParticleEditScene() = default;

	void Init() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

	void ImGuiDebug();

private:

	void AddFloorGrid();

private:

	std::unique_ptr<Camera> camera_ = nullptr;
	std::unique_ptr<Skybox> skybox_ = nullptr;

	LineRenderer debugLine_;
	ParticleEditor particleEditor_;

	Vector3 previewPos_{ 0.0f, 1.5f, 0.0f };

	float cameraMoveSpeed_ = 0.2f;
	float cameraRotSpeed_ = 0.02f;

	bool showGrid_ = true;
	bool showEditor_ = true;
};