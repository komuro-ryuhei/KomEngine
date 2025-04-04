#pragma once

#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"

#include "Engine/lib/Input/Input.h"

class SceneManager;

class IScene {

public:

	virtual ~IScene() = default;

	virtual void Init();

	virtual void Update();

	virtual void Draw();

	virtual void Finalize();

public:

	virtual void SetSceneManager(SceneManager* sceneManager);

public:
	// シーンマネージャー
	SceneManager* sceneManager_ = nullptr;
};