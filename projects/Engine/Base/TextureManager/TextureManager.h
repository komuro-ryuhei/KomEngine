#pragma once
// C++
#include <d3d12.h>
#include <externals/DirectXTex/DirectXTex.h>
#include <string>
#include <unordered_map>

// MyClass
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/SrvManager/SrvManager.h"
#include "Engine/lib/ComPtr/ComPtr.h"

// テクスチャマネージャー
class TextureManager {

public:
	// シングルトンインスタンスの取得
	static TextureManager* GetInstance();

	void Init(SrvManager* srvManager);
	// 終了処理
	void Finalize();

public:
	/// <summary>
	/// テクスチャファイルの読み込み
	/// </summary>
	void LoadTexture(const std::string& filePath);

	/// <summary>
	/// テクスチャ作成
	/// </summary>
	ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

	/// <summary>
	///
	/// </summary>
	ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

	// SRVインデックスの開始番号
	uint32_t GetTextureIndexByFilePath(const std::string& filePath);

	/// <summary>
	/// ハンドル取得
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	// メタデータ取得
	const DirectX::TexMetadata& GetMetaData(const std::string& filePath);

	// テクスチャ番号からGPUハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

	uint32_t GetSrvIndex(const std::string& filePath);

public:
	static TextureManager* instance;

	// SRVインデックスの開始番号
	static uint32_t kSRVIndexTop_;

private:
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;

	// テクスチャ1枚分のデータ
	struct TextureData {
		std::string filePath;
		DirectX::TexMetadata metaData;
		ComPtr<ID3D12Resource> resource;
		ComPtr<ID3D12Resource> intermediateResource;
		uint32_t srvIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};

	SrvManager* srvManager_ = nullptr;

	// テクスチャデータ
	// std::vector<TextureData> textureDatas;

	std::unordered_map<std::string, TextureData> textureDatas;
};