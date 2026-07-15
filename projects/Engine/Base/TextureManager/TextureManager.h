#pragma once
// C++
#include <d3d12.h>
#include <externals/DirectXTex/DirectXTex.h>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <vector>

// MyClass
#include "Engine/Base/DirectXCommon/DirectXCommon.h"
#include "Engine/Base/SrvManager/SrvManager.h"
#include "Engine/lib/ComPtr/ComPtr.h"

namespace fs = std::filesystem;

/// <summary>
/// テクスチャ管理クラス
/// </summary>
class TextureManager {

public:

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="srvManager"> SRVManager </param>
	void Init(SrvManager* srvManager);

	/// <summary>
	/// テクスチャファイルの読み込み
	/// </summary>
	void LoadTexture(const std::string& filePath);

	/// <summary>
	/// テクスチャ作成
	/// </summary>
	ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

	/// <summary>
	/// テクスチャデータのアップロード
	/// </summary>
	ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

	// SRVインデックスの開始番号
	uint32_t GetTextureIndexByFilePath(const std::string& filePath);

	/// <summary>
	/// ハンドル取得
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	// メタデータ取得
	const DirectX::TexMetadata& GetMetaData(const std::string& filePath);

	// テクスチャ番号からGPUハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

	// テクスチャ番号からCPUハンドルを取得
	uint32_t GetSrvIndex(const std::string& filePath);

public:

	// SRVインデックスの開始番号
	static uint32_t kSRVIndexTop_;

	/// <summary>
	/// PNG/JPGとDDSの読み込み時間を比較するための結果
	/// </summary>
	struct TextureBenchmarkResult {

		// 比較できたPNG/DDSの組数
		uint32_t texturePairCount = 0;

		// 実行回数
		uint32_t iterationCount = 0;

		// 1回あたりの平均値
		double pngFileLoadMs = 0.0;
		double pngMipMapMs = 0.0;
		double pngTotalMs = 0.0;

		double ddsFileLoadMs = 0.0;
		double ddsMipMapMs = 0.0;
		double ddsTotalMs = 0.0;
	};

	/// <summary>
	/// 指定されたPNG/JPGと、同名のDDSを比較計測する
	/// 通常のテクスチャキャッシュやSRVには影響しない
	/// </summary>
	/// <param name="sourcePaths">
	/// PNGまたはJPG側のパス一覧
	/// </param>
	/// <param name="iterationCount">
	/// 計測回数
	/// </param>
	TextureBenchmarkResult RunTextureBenchmark(
		const std::vector<std::string>& sourcePaths,
		uint32_t iterationCount = 5
	) const;

private:

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

private:

	/// <summary>
	/// 
	/// </summary>
	std::string PreferDDSPath(const std::string& requestPath) const;
};