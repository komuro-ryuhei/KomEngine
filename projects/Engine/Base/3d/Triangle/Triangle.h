#pragma once

// MyClass
#include "Engine/lib/ComPtr/ComPtr.h"
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/Mesh/Mesh.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"
#include "Engine/Base/WinApp/WinApp.h"

// C++
#include <memory>

class Triangle {

public:
	void Initialize(DirectXCommon* dxCommon, PipelineManager* pipelineManager);

	void Draw();

private:
	// DirectXCommon
	DirectXCommon* dxCommon_ = nullptr;
	// Pipeline
	PipelineManager* pipelineManager_ = nullptr;

	// Mesh
	std::unique_ptr<Mesh> mesh_ = nullptr;
};