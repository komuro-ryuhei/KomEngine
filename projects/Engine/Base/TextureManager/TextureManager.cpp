#include "TextureManager.h"

#include "Engine/Base/System/System.h"
#include "externals/DirectXTex/d3dx12.h"
#include <vector>

uint32_t TextureManager::kSRVIndexTop_ = 1;

void TextureManager::Init(SrvManager* srvManager) {

	srvManager_ = srvManager;

	// SRVの数と同様
	textureDatas.reserve(DirectXCommon::kMaxSRVCount);
}

std::string TextureManager::PreferDDSPath(const std::string& requestPath)
{
	fs::path p(requestPath);
	if (!p.has_extension()) return requestPath;

	fs::path dds = p;
	dds.replace_extension(".dds");

	if (fs::exists(dds)) {
		return dds.string();
	}
	return requestPath;
}

void TextureManager::LoadTexture(const std::string& filePath) {

	// ★追加：ddsがあればそっちに寄せる
	const std::string actualPath = PreferDDSPath(filePath);

	if (textureDatas.contains(actualPath)) {
		return;
	}

	assert(srvManager_->CanAllocate());

	TextureData textureData;
	textureData.filePath = actualPath;
	std::wstring filePathW = StringUtility::ConvertString(textureData.filePath);

	DirectX::ScratchImage image{};
	HRESULT hr;

	if (filePathW.ends_with(L".dds")) {
		hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	} else {
		hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}

	assert(SUCCEEDED(hr));

	DirectX::ScratchImage mipImage{};

	if (DirectX::IsCompressed(image.GetMetadata().format)) {
		mipImage = std::move(image);
	} else {
		hr = DirectX::GenerateMipMaps(
			image.GetImages(), image.GetImageCount(), image.GetMetadata(),
			DirectX::TEX_FILTER_SRGB, 4, mipImage
		);
		assert(SUCCEEDED(hr));
	}

	textureData.metaData = mipImage.GetMetadata();
	textureData.resource = CreateTextureResource(KomEngine::System::GetDxCommon()->GetDevice(), textureData.metaData);

	textureData.srvIndex = srvManager_->Allocate();
	textureData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
	textureData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);

	const bool isCubeMap =
		textureData.metaData.dimension == DirectX::TEX_DIMENSION_TEXTURE2D &&
		textureData.metaData.arraySize == 6 &&
		(textureData.metaData.miscFlags & DirectX::TEX_MISC_TEXTURECUBE);

	if (isCubeMap) {
		srvManager_->CreateSRVforTextureCube(
			textureData.srvIndex, textureData.resource.Get(),
			textureData.metaData.format, static_cast<UINT>(textureData.metaData.mipLevels)
		);
	} else {
		srvManager_->CreateSRVforTexture2D(
			textureData.srvIndex, textureData.resource.Get(),
			textureData.metaData.format, static_cast<UINT>(textureData.metaData.mipLevels)
		);
	}

	textureData.intermediateResource = UploadTextureData(textureData.resource.Get(), mipImage);

	// キーも actualPath（= textureData.filePath）で統一
	textureDatas[textureData.filePath] = std::move(textureData);
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
	    D3D12_RESOURCE_STATE_COPY_DEST   , // 初回のResourceState
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