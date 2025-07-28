#include "System.h"

// MyClass
#include "Engine/Base/ImGuiManager/ImGuiManager.h"
#include "Engine/Base/ModelManager/ModelManager.h"
#include "Engine/Base/SrvManager/SrvManager.h"
#include "Engine/Base/TextureManager/TextureManager.h"

#include "externals/imgui/imgui.h"

#include <cassert>
#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif // _DEBUG

/*==================================================================================*/
// システム全体の初期化

// WindowsAPI
std::unique_ptr<WinApp> winApp_ = nullptr;
// DirectXCommon
std::unique_ptr<DirectXCommon> dxCommon_ = nullptr;
// Input
std::unique_ptr<Input> input_ = nullptr;
// Mesh
std::unique_ptr<Light> light_ = nullptr;
// SrvManager
std::unique_ptr<SrvManager> srvManager_ = nullptr;
// ImGuiManager
std::unique_ptr<ImGuiManager> imguiManager_ = nullptr;
// OffscreenRendering
std::unique_ptr<OffscreenRendering> offscreenRendering_ = nullptr;

DirectXCommon* System::GetDxCommon() { return dxCommon_.get(); }

Input* System::GetInput() { return input_.get(); }

SrvManager* System::GetSrvManager() { return srvManager_.get(); }

Light* System::GetLight() { return light_.get(); }

WinApp* System::GetWinApp() { return winApp_.get(); }

OffscreenRendering* System::GetOffscreenRendering() { return offscreenRendering_.get(); }

void System::Initialize(const char* title, int width, int height) {

	winApp_ = std::make_unique<WinApp>();

	// ゲームウインドウの作成
	std::string titleWithVersion = std::string(title);
	auto&& titleString = StringUtility::ConvertString(titleWithVersion);
	winApp_->CreateGameWindow(titleString.c_str(), WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME), width, height);

	// DirectXの初期化処理
	dxCommon_ = std::make_unique<DirectXCommon>();
	dxCommon_->Initialize(winApp_.get());

	//
	offscreenRendering_ = std::make_unique<OffscreenRendering>();
	offscreenRendering_->Init();

	// SrvManager
	srvManager_ = std::make_unique<SrvManager>();
	srvManager_->Init();

	// Inputの初期化
	input_ = std::make_unique<Input>();
	input_->Initialize(winApp_.get());

	// TextureManager
	TextureManager::GetInstance()->Init(srvManager_.get());

	ModelManager::GetInstance()->Init(dxCommon_.get());

	// Mesh
	light_ = std::make_unique<Light>();
	light_->LightSetting();

	imguiManager_ = std::make_unique<ImGuiManager>();
	imguiManager_->Init(winApp_.get(), dxCommon_.get());
}

bool System::ProcessMessage() { return winApp_->ProcessMessage(); }

void System::BeginFrame() {

	offscreenRendering_->RenderToTexture();

	srvManager_->PreDraw();

	imguiManager_->Begin();

	input_->Update();

#ifdef _DEBUG

	// mesh_->ImGuiDebug();

#endif // _DEBUG
}

void System::Update() {}

void System::EndFrame() {

	// DirectX描画前処理
	dxCommon_->PreDraw();

	// Offscreen描画処理
	offscreenRendering_->OffscreenBarrier();
	offscreenRendering_->Draw();

	imguiManager_->End();
	// ImGui描画処理
#ifdef _DEBUG
	imguiManager_->Draw();
#endif // DEBUG

	// 描画終了
	offscreenRendering_->PostDraw();
	dxCommon_->PostDraw();
}

void System::Finalize() {

	winApp_->TerminateGameWindow();

	winApp_.reset();
	dxCommon_.reset();
	input_.reset();
	light_.reset();

	//
	TextureManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();

	imguiManager_->Finalize();
}

bool System::PushKey(BYTE keyNumber) { return input_->PushKey(keyNumber); }

bool System::TriggerKey(BYTE keyNumber) { return input_->TriggerKey(keyNumber); }