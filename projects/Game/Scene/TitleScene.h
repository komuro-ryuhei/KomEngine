#pragma once

#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"

#include "Engine/Base/2d/Sprite/Sprite.h"
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/Audio/Audio.h"
#include "Engine/Base/Camera/Camera.h"
#include "Engine/lib/Input/Input.h"

#include "Game/Scene/GameScene.h"
#include "Game/Scene/IScene.h"
#include "Game/Scene/SceneManager.h"

class TitleScene : public IScene {

public:
	void Init() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:
	// Camera
	std::unique_ptr<Camera> camera_ = nullptr;
	// Sprite
	std::unique_ptr<Sprite> sprite_ = nullptr;
};