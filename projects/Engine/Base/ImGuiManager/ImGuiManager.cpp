#include "ImGuiManager.h"

#ifdef USE_IMGUI

#include <externals/imgui/imgui.h>
#include <externals/imgui/imgui_impl_dx12.h>
#include <externals/imgui/imgui_impl_win32.h>

#endif

void ImGuiManager::Init(WinApp* winApp) {

#ifdef USE_IMGUI

	HRESULT hr;

	winApp_ = winApp;

	// ImGuiのコンテキストを生成
	ImGui::CreateContext();
	// ImGuiのスタイルを設定
	ImGui::StyleColorsDark();

	{
		ImGuiIO& io = ImGui::GetIO();

		const char* fontPath = "Resources/fonts/NotoSansJP-Regular.otf";
		float fontSize = 20.0f;

		ImFontConfig config;
		config.MergeMode = false;
		config.PixelSnapH = true;

		static const ImWchar japaneseRange[] = {
			0x0020, 0x00FF,
			0x3000, 0x30FF,
			0x4E00, 0x9FAF,
			0,
		};

		io.Fonts->AddFontFromFileTTF(fontPath, fontSize, &config, japaneseRange);
	}

	// Win32用の初期化
	ImGui_ImplWin32_Init(winApp_->GetHwnd());

	// デスクリプタヒープ設定
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	// デスクリプタヒープ生成
	hr = System::GetDxCommon()->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap_));
	assert(SUCCEEDED(hr));

	// DX12用の初期化
	ImGui_ImplDX12_Init(
		System::GetDxCommon()->GetDevice(), static_cast<int>(System::GetDxCommon()->GetBackBufferCount()), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, srvHeap_.Get(), srvHeap_->GetCPUDescriptorHandleForHeapStart(),
	    srvHeap_->GetGPUDescriptorHandleForHeapStart());

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

#endif
}

void ImGuiManager::Finalize() {
	
#ifdef USE_IMGUI

	// 後始末
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

#endif

	// デスクリプタヒープを解放
	srvHeap_.Reset();
}

void ImGuiManager::Begin() {

#ifdef USE_IMGUI

	// ImGuiフレーム開始
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

#endif
}

void ImGuiManager::End() {

#ifdef USE_IMGUI

	// 描画前準備
	ImGui::Render();

#endif
}

void ImGuiManager::Draw() {
	
	ID3D12GraphicsCommandList* commandList = System::GetDxCommon()->GetCommandList();

	// デスクリプタヒープ
	ID3D12DescriptorHeap* ppHeaps[] = {srvHeap_.Get()};
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

#ifdef USE_IMGUI

	// 描画コマンドを発行
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

#endif
}