#include "TextureManager.h"

#include "Engine/Base/System/System.h"
#include "externals/DirectXTex/d3dx12.h"
#include <vector>

TextureManager* TextureManager::instance = nullptr;
uint32_t TextureManager::kSRVIndexTop_ = 1;

TextureManager* TextureManager::GetInstance() {
	if (instance == nullptr) {
		instance = new TextureManager;
	}
	return instance;
}

void TextureManager::Init(SrvManager* srvManager) {

	srvManager_ = srvManager;

	// SRVの数と同様
	textureDatas.reserve(DirectXCommon::kMaxSRVCount);
}

void TextureManager::Finalize() {

	delete instance;
	instance = nullptr;
}

void TextureManager::LoadTexture(const std::string& filePath) {

	if (textureDatas.contains(filePath)) {
		return;
	}

	assert(srvManager_->CanAllocate());

	TextureData textureData;
	textureData.filePath = std::move(filePath); // ファイルパスをムーブ
	std::wstring filePathW = StringUtility::ConvertString(textureData.filePath);

	DirectX::ScratchImage image{};
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	DirectX::ScratchImage mipImage{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImage);
	assert(SUCCEEDED(hr));

	textureData.metaData = mipImage.GetMetadata();
	textureData.resource = CreateTextureResource(System::GetDxCommon()->GetDevice(), textureData.metaData);

	textureData.srvIndex = srvManager_->Allocate();
	textureData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
	textureData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);

	srvManager_->CreateSRVforTexture2D(textureData.srvIndex, textureData.resource.Get(), textureData.metaData.format, static_cast<UINT>(textureData.metaData.mipLevels));

	textureData.intermediateResource = UploadTextureData(textureData.resource.Get(), mipImage);

	// ここでムーブ代入を使用
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
ID3D12Resource* TextureManager::UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages) {

	// 
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(System::GetDxCommon()->GetDevice(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	ComPtr<ID3D12Resource> intermediateResource = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), intermediateSize);
	UpdateSubresources(System::GetDxCommon()->GetCommandList(), texture, intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());
	// Textureへの転送は後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	System::GetDxCommon()->GetCommandList()->ResourceBarrier(1, &barrier);

	return intermediateResource.Get();
}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath) {

	// テクスチャが存在するかをチェック
	if (textureDatas.contains(filePath)) {
		// 存在する場合、そのテクスチャのインデックスを返す
		return textureDatas[filePath].srvIndex;
	}

	// 存在しない場合
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

	// 範囲外指定違反チェック
	assert(textureDatas.contains(filePath));

	TextureData& textureData = textureDatas[filePath];

	return textureData.srvHandleGPU;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath) {

	// 範囲外指定違反チェック
	assert(textureDatas.contains(filePath));

	TextureData& textureData = textureDatas[filePath];

	return textureData.metaData;
}

uint32_t TextureManager::GetSrvIndex(const std::string& filePath) {

	// 指定されたファイルパスが存在するかチェック
	assert(textureDatas.contains(filePath));

	// 該当テクスチャデータを取得
	TextureData& textureData = textureDatas[filePath];
	return textureData.srvIndex;
}
