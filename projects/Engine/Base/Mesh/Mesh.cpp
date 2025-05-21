#include "Mesh.h"

#include "Engine/Base/System/System.h"
#include "externals/imgui/imgui.h"
#include <numbers>

D3D12_VERTEX_BUFFER_VIEW Mesh::GetVBV() const { return vertexBufferView; }

ID3D12Resource* Mesh::GetMateialResource() const { return materialResource_.Get(); }

ID3D12Resource* Mesh::GetLightResource() const { return materialResourceLight.Get(); }

ID3D12Resource* Mesh::GetPhongLightResource() const { return materialResourcePhong.Get(); }

ID3D12Resource* Mesh::GetPointLightResource() const { return materialResourcePoint.Get(); }

ID3D12Resource* Mesh::GetSpotLightResource() const { return materialResourceSpot.Get(); }

ComPtr<ID3D12Resource> Mesh::CreateVertexResource(size_t sizeInBytes) {

	HRESULT hr;

	// 頂点リソース用のヒープの設定
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	// 頂点リソースの設定
	// バッファリソース
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes; // リソースのサイズ。今回はVector4を3頂点分
	// バッファの場合はこれらは1にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	// バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	// 実際に頂点リソースを作る
	hr = System::GetDxCommon()->GetDevice()->CreateCommittedResource(
	    &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));

	return vertexResource;
}

void Mesh::CreateVertexBufferView() {

	// リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(Vector4) * 3;
	// 1頂点当あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(Vector4);
}

void Mesh::CreateMaterialResource() {

	materialResource_ = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(Vector4));
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = Vector4(1.0, 1.0f, 1.0f, 1.0f);
}

void Mesh::WriteDateForResource() {

	// 頂点リソースにデータを書き込む
	Vector4* vertexData = nullptr;
	// 書き込むアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	// 左下
	vertexData[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
	// 上
	vertexData[1] = {0.0f, 0.5f, 0.0f, 1.0f};
	// 右下
	vertexData[2] = {0.5f, -0.5f, 0.0f, 1.0f};

	CreateMaterialResource();
}

void Mesh::LightSetting() {

	// Light用のマテリアルリソースを作る
	materialResourceLight = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(DirectionalLight));
	materialResourceLight->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	directionalLightData->direction = {0.0f, -1.0f, 0.0f};
	directionalLightData->intensity = 0.0f;

	// Phong用のマテリアルリソースを作る
	materialResourcePhong = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(CameraForGPU));
	materialResourcePhong->Map(0, nullptr, reinterpret_cast<void**>(&phongLightData));

	phongLightData->worldPosition = {0.0f, 4.0f, -10.0f};

	// PointLight用のマテリアルリソースを作る
	materialResourcePoint = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(PointLight));
	materialResourcePoint->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData));

	pointLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	pointLightData->position = {0.0f, 2.0f, 0.0f};
	pointLightData->intensity = 0.0f;
	pointLightData->radius = 3.0f;
	pointLightData->decay = 2.0f;

	// PointLight用のマテリアルリソースを作る
	materialResourceSpot = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(SpotLight));
	materialResourceSpot->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData));

	spotLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	spotLightData->position = {2.0f, 1.25f, 0.0f};
	spotLightData->distance = 7.0f;
	spotLightData->direction = MyMath::Normalize({-1.0f, -1.0f, 0.0f});
	spotLightData->intensity = 4.0f;
	spotLightData->decay = 2.0f;
	spotLightData->casAngle = std::cos(std::numbers::pi_v<float> / 3.0f);
	spotLightData->cosFalloffStart = 1.0f;
}

void Mesh::ImGuiDebug() {

	ImGui::Begin("Light");

	ImGui::ColorEdit4("PointLightColor", &pointLightData->color.x);
	ImGui::DragFloat3("PointLightPosition", &pointLightData->position.x, 0.01f);
	ImGui::SliderFloat("PointLightIntencity", &pointLightData->intensity, 0.0f, 10.0f);
	ImGui::SliderFloat("PointLightRadius", &pointLightData->radius, 0.0f, 10.0f);
	ImGui::SliderFloat("PointLightDecay", &pointLightData->decay, 0.0f, 10.0f);

	ImGui::ColorEdit4("SpotLightColor", &spotLightData->color.x);
	ImGui::DragFloat3("SpotoPsition", &spotLightData->position.x, 0.01f);
	ImGui::DragFloat3("SpotLightIntencity", &spotLightData->distance, 0.0f, 10.0f);
	ImGui::SliderFloat("SpotLightRadius", &spotLightData->intensity, 0.0f, 10.0f);
	ImGui::SliderFloat("SpotLightDecay", &spotLightData->decay, 0.0f, 10.0f);
	ImGui::SliderFloat("SpotLightCosAngle", &spotLightData->casAngle, -2.0f, 1.0f);
	ImGui::SliderFloat("SpotLightCosStartAngle", &spotLightData->cosFalloffStart, 1.0f, 10.0f);

	ImGui::End();
}