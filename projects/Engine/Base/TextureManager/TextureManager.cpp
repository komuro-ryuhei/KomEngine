#include "TextureManager.h"

#include "Engine/Base/System/System.h"
#include "externals/DirectXTex/d3dx12.h"
#include <vector>
#include <chrono>

uint32_t TextureManager::kSRVIndexTop_ = 1;

void TextureManager::Init(SrvManager* srvManager) {

	srvManager_ = srvManager;

	// SRVの数と同様
	textureDatas.reserve(DirectXCommon::kMaxSRVCount);
}

std::string TextureManager::PreferDDSPath(const std::string& requestPath) const {

	fs::path path(requestPath);

	if (!path.has_extension()) {
		return requestPath;
	}

	fs::path ddsPath = path;
	ddsPath.replace_extension(".dds");

	// 同名のDDSがあれば、通常実行ではDDSを優先する
	if (fs::exists(ddsPath)) {
		return ddsPath.string();
	}

	return requestPath;
}

void TextureManager::LoadTexture(const std::string& filePath) {

	const std::string actualPath =
		PreferDDSPath(filePath);

	// すでに読み込み済みなら何もしない
	if (textureDatas.contains(actualPath)) {
		return;
	}

	assert(srvManager_->CanAllocate());

	TextureData textureData{};
	textureData.filePath = actualPath;

	std::wstring filePathW =
		StringUtility::ConvertString(
			textureData.filePath
		);

	DirectX::ScratchImage image{};
	HRESULT hr = S_OK;

	// DDSと、それ以外の画像で読み込み処理を分ける
	if (filePathW.ends_with(L".dds")) {

		hr = DirectX::LoadFromDDSFile(
			filePathW.c_str(),
			DirectX::DDS_FLAGS_NONE,
			nullptr,
			image
		);

	}
	else {

		hr = DirectX::LoadFromWICFile(
			filePathW.c_str(),
			DirectX::WIC_FLAGS_FORCE_SRGB,
			nullptr,
			image
		);
	}

	assert(SUCCEEDED(hr));

	DirectX::ScratchImage mipImage{};

	// 圧縮済みDDSは、そのままミップ情報を利用する
	if (DirectX::IsCompressed(
		image.GetMetadata().format)) {

		mipImage = std::move(image);

	}
	else {

		hr = DirectX::GenerateMipMaps(
			image.GetImages(),
			image.GetImageCount(),
			image.GetMetadata(),
			DirectX::TEX_FILTER_SRGB,
			4,
			mipImage
		);

		assert(SUCCEEDED(hr));
	}

	textureData.metaData =
		mipImage.GetMetadata();

	textureData.resource =
		CreateTextureResource(
			KomEngine::System::GetDxCommon()->GetDevice(),
			textureData.metaData
		);

	textureData.srvIndex =
		srvManager_->Allocate();

	textureData.srvHandleCPU =
		srvManager_->GetCPUDescriptorHandle(
			textureData.srvIndex
		);

	textureData.srvHandleGPU =
		srvManager_->GetGPUDescriptorHandle(
			textureData.srvIndex
		);

	const bool isCubeMap =
		textureData.metaData.dimension ==
		DirectX::TEX_DIMENSION_TEXTURE2D &&
		textureData.metaData.arraySize == 6 &&
		(
			textureData.metaData.miscFlags &
			DirectX::TEX_MISC_TEXTURECUBE
			);

	if (isCubeMap) {

		srvManager_->CreateSRVforTextureCube(
			textureData.srvIndex,
			textureData.resource.Get(),
			textureData.metaData.format,
			static_cast<UINT>(
				textureData.metaData.mipLevels
				)
		);

	}
	else {

		srvManager_->CreateSRVforTexture2D(
			textureData.srvIndex,
			textureData.resource.Get(),
			textureData.metaData.format,
			static_cast<UINT>(
				textureData.metaData.mipLevels
				)
		);
	}

	textureData.intermediateResource =
		UploadTextureData(
			textureData.resource.Get(),
			mipImage
		);

	textureDatas[textureData.filePath] =
		std::move(textureData);
}

ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata) {

	// metadaraを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);                             // Textureの幅
	resourceDesc.Height = UINT(metadata.height);                           // Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);                   // mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);            // 奥行き or 配列Textureの配列
	resourceDesc.Format = metadata.format;                                 // TextureのFormat
	resourceDesc.SampleDesc.Count = 1;                                     // サンプリングカウント
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;                         // 細かい設定を行う
	//heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // WriteBackポリシーでCPUアクセス可能
	//heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;          // プロセッサの近くに配置

	// Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,                   // Heapの設定
		D3D12_HEAP_FLAG_NONE,              // Heapの特殊な設定
		&resourceDesc,                     // Resourceの設定
		D3D12_RESOURCE_STATE_COPY_DEST, // 初回のResourceState
		nullptr,                           // Clear最速値
		IID_PPV_ARGS(&resource));          // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));
	return resource;
}

[[nodiscard]]
ComPtr<ID3D12Resource> TextureManager::UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages) {

	// 
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(KomEngine::System::GetDxCommon()->GetDevice(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	ComPtr<ID3D12Resource> intermediateResource = KomEngine::System::GetDxCommon()->CreateBufferResource(KomEngine::System::GetDxCommon()->GetDevice(), intermediateSize);
	UpdateSubresources(KomEngine::System::GetDxCommon()->GetCommandList(), texture, intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());
	// Textureへの転送は後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	KomEngine::System::GetDxCommon()->GetCommandList()->ResourceBarrier(1, &barrier);

	return intermediateResource;
}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath) {

	const std::string actualPath = PreferDDSPath(filePath);

	if (textureDatas.contains(actualPath)) {
		return textureDatas[actualPath].srvIndex;
	}

	assert(0);
	return 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);

	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);

	return handleGPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath) {

	const std::string actualPath = PreferDDSPath(filePath);

	assert(textureDatas.contains(actualPath));

	TextureData& textureData = textureDatas[actualPath];
	return textureData.srvHandleGPU;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath) {

	const std::string actualPath = PreferDDSPath(filePath);

	assert(textureDatas.contains(actualPath));

	TextureData& textureData = textureDatas[actualPath];
	return textureData.metaData;
}

uint32_t TextureManager::GetSrvIndex(const std::string& filePath) {

	const std::string actualPath = PreferDDSPath(filePath);

	assert(textureDatas.contains(actualPath));

	TextureData& textureData = textureDatas[actualPath];
	return textureData.srvIndex;
}

TextureManager::TextureBenchmarkResult
TextureManager::RunTextureBenchmark(
	const std::vector<std::string>& sourcePaths,
	uint32_t iterationCount
) const {

	using Clock = std::chrono::steady_clock;

	TextureBenchmarkResult result{};

	if (sourcePaths.empty() || iterationCount == 0) {

		OutputDebugStringA(
			"[TextureBenchmark] No texture or iteration specified.\n"
		);

		return result;
	}

	// 1枚分のCPU側読み込み時間
	struct MeasureResult {

		bool succeeded = false;
		bool compressed = false;

		double fileLoadMs = 0.0;
		double mipMapMs = 0.0;
		double totalMs = 0.0;
	};

	auto measureTexture =
		[](const fs::path& path) -> MeasureResult {

		MeasureResult measure{};

		if (!fs::exists(path)) {

			std::string message =
				"[TextureBenchmark] File not found: " +
				path.string() +
				"\n";

			OutputDebugStringA(message.c_str());
			return measure;
		}

		const auto totalStart =
			Clock::now();

		DirectX::ScratchImage image{};
		HRESULT hr = E_FAIL;

		const std::wstring pathW =
			path.wstring();

		const bool isDDS =
			path.extension() == ".dds" ||
			path.extension() == ".DDS";

		// ファイルのデコード・展開時間
		const auto fileLoadStart =
			Clock::now();

		if (isDDS) {

			hr = DirectX::LoadFromDDSFile(
				pathW.c_str(),
				DirectX::DDS_FLAGS_NONE,
				nullptr,
				image
			);

		}
		else {

			hr = DirectX::LoadFromWICFile(
				pathW.c_str(),
				DirectX::WIC_FLAGS_FORCE_SRGB,
				nullptr,
				image
			);
		}

		const auto fileLoadEnd =
			Clock::now();

		measure.fileLoadMs =
			std::chrono::duration<double, std::milli>(
				fileLoadEnd - fileLoadStart
			).count();

		if (FAILED(hr)) {

			std::string message =
				"[TextureBenchmark] Load failed: " +
				path.string() +
				"\n";

			OutputDebugStringA(message.c_str());
			return measure;
		}

		measure.compressed =
			DirectX::IsCompressed(
				image.GetMetadata().format
			);

		DirectX::ScratchImage mipImage{};

		// 実際のLoadTextureと同じ条件で測定
		const auto mipMapStart =
			Clock::now();

		if (measure.compressed) {

			// 圧縮済みDDSはGenerateMipMapsを通さない
			mipImage = std::move(image);

		}
		else {

			hr = DirectX::GenerateMipMaps(
				image.GetImages(),
				image.GetImageCount(),
				image.GetMetadata(),
				DirectX::TEX_FILTER_SRGB,
				4,
				mipImage
			);
		}

		const auto mipMapEnd =
			Clock::now();

		measure.mipMapMs =
			std::chrono::duration<double, std::milli>(
				mipMapEnd - mipMapStart
			).count();

		if (FAILED(hr)) {

			std::string message =
				"[TextureBenchmark] GenerateMipMaps failed: " +
				path.string() +
				"\n";

			OutputDebugStringA(message.c_str());
			return measure;
		}

		const auto totalEnd =
			Clock::now();

		measure.totalMs =
			std::chrono::duration<double, std::milli>(
				totalEnd - totalStart
			).count();

		measure.succeeded = true;
		return measure;
		};

	std::vector<fs::path> validSourcePaths;

	for (const std::string& sourcePath : sourcePaths) {

		fs::path source = sourcePath;
		fs::path dds = source;
		dds.replace_extension(".dds");

		// 両方存在する画像だけ比較対象にする
		if (!fs::exists(source)) {

			std::string message =
				"[TextureBenchmark] Source not found: " +
				source.string() +
				"\n";

			OutputDebugStringA(message.c_str());
			continue;
		}

		if (!fs::exists(dds)) {

			std::string message =
				"[TextureBenchmark] DDS not found: " +
				dds.string() +
				"\n";

			OutputDebugStringA(message.c_str());
			continue;
		}

		validSourcePaths.push_back(source);
	}

	if (validSourcePaths.empty()) {

		OutputDebugStringA(
			"[TextureBenchmark] No valid PNG/DDS pairs.\n"
		);

		return result;
	}

	double pngFileLoadTotal = 0.0;
	double pngMipMapTotal = 0.0;
	double pngTotal = 0.0;

	double ddsFileLoadTotal = 0.0;
	double ddsMipMapTotal = 0.0;
	double ddsTotal = 0.0;

	uint32_t successfulIterationCount = 0;

	for (uint32_t iteration = 0;
		iteration < iterationCount;
		++iteration) {

		double currentPngFileLoad = 0.0;
		double currentPngMipMap = 0.0;
		double currentPngTotal = 0.0;

		double currentDdsFileLoad = 0.0;
		double currentDdsMipMap = 0.0;
		double currentDdsTotal = 0.0;

		bool iterationSucceeded = true;

		// 毎回同じ順序による偏りを減らすため、
		// 偶数回と奇数回でPNG/DDSの計測順を入れ替える
		const bool pngFirst =
			(iteration % 2 == 0);

		for (const fs::path& source :
			validSourcePaths) {

			fs::path dds = source;
			dds.replace_extension(".dds");

			MeasureResult pngMeasure{};
			MeasureResult ddsMeasure{};

			if (pngFirst) {

				pngMeasure =
					measureTexture(source);

				ddsMeasure =
					measureTexture(dds);

			}
			else {

				ddsMeasure =
					measureTexture(dds);

				pngMeasure =
					measureTexture(source);
			}

			if (!pngMeasure.succeeded ||
				!ddsMeasure.succeeded) {

				iterationSucceeded = false;
				break;
			}

			currentPngFileLoad +=
				pngMeasure.fileLoadMs;

			currentPngMipMap +=
				pngMeasure.mipMapMs;

			currentPngTotal +=
				pngMeasure.totalMs;

			currentDdsFileLoad +=
				ddsMeasure.fileLoadMs;

			currentDdsMipMap +=
				ddsMeasure.mipMapMs;

			currentDdsTotal +=
				ddsMeasure.totalMs;
		}

		if (!iterationSucceeded) {
			continue;
		}

		++successfulIterationCount;

		pngFileLoadTotal +=
			currentPngFileLoad;

		pngMipMapTotal +=
			currentPngMipMap;

		pngTotal +=
			currentPngTotal;

		ddsFileLoadTotal +=
			currentDdsFileLoad;

		ddsMipMapTotal +=
			currentDdsMipMap;

		ddsTotal +=
			currentDdsTotal;

		std::string iterationOutput =
			"\n[TextureBenchmark] Iteration " +
			std::to_string(iteration + 1) +
			"\nPNG File Load : " +
			std::to_string(currentPngFileLoad) +
			" ms" +
			"\nPNG MipMap    : " +
			std::to_string(currentPngMipMap) +
			" ms" +
			"\nPNG Total     : " +
			std::to_string(currentPngTotal) +
			" ms" +
			"\nDDS File Load : " +
			std::to_string(currentDdsFileLoad) +
			" ms" +
			"\nDDS MipMap    : " +
			std::to_string(currentDdsMipMap) +
			" ms" +
			"\nDDS Total     : " +
			std::to_string(currentDdsTotal) +
			" ms\n";

		OutputDebugStringA(
			iterationOutput.c_str()
		);
	}

	if (successfulIterationCount == 0) {

		OutputDebugStringA(
			"[TextureBenchmark] All iterations failed.\n"
		);

		return result;
	}

	const double iterationDivisor =
		static_cast<double>(
			successfulIterationCount
			);

	result.texturePairCount =
		static_cast<uint32_t>(
			validSourcePaths.size()
			);

	result.iterationCount =
		successfulIterationCount;

	result.pngFileLoadMs =
		pngFileLoadTotal /
		iterationDivisor;

	result.pngMipMapMs =
		pngMipMapTotal /
		iterationDivisor;

	result.pngTotalMs =
		pngTotal /
		iterationDivisor;

	result.ddsFileLoadMs =
		ddsFileLoadTotal /
		iterationDivisor;

	result.ddsMipMapMs =
		ddsMipMapTotal /
		iterationDivisor;

	result.ddsTotalMs =
		ddsTotal /
		iterationDivisor;

	const double reductionMs =
		result.pngTotalMs -
		result.ddsTotalMs;

	const double reductionPercent =
		result.pngTotalMs > 0.0
		? reductionMs /
		result.pngTotalMs *
		100.0
		: 0.0;

	std::string totalOutput =
		"\n"
		"========================================\n"
		" Texture Benchmark Average\n"
		"========================================\n"
		"Texture pairs : " +
		std::to_string(result.texturePairCount) +
		"\nIterations    : " +
		std::to_string(result.iterationCount) +
		"\n"
		"\nPNG File Load : " +
		std::to_string(result.pngFileLoadMs) +
		" ms" +
		"\nPNG MipMap    : " +
		std::to_string(result.pngMipMapMs) +
		" ms" +
		"\nPNG Total     : " +
		std::to_string(result.pngTotalMs) +
		" ms" +
		"\n"
		"\nDDS File Load : " +
		std::to_string(result.ddsFileLoadMs) +
		" ms" +
		"\nDDS MipMap    : " +
		std::to_string(result.ddsMipMapMs) +
		" ms" +
		"\nDDS Total     : " +
		std::to_string(result.ddsTotalMs) +
		" ms" +
		"\n"
		"\nReduction     : " +
		std::to_string(reductionMs) +
		" ms" +
		"\nReduction rate: " +
		std::to_string(reductionPercent) +
		" %\n"
		"========================================\n";

	OutputDebugStringA(
		totalOutput.c_str()
	);

	return result;
}