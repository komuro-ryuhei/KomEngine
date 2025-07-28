#include "Light.h"

#include "Engine/Base/System/System.h"
#include <numbers>

#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif // _DEBUG

ID3D12Resource* Light::GetLightResource() const { return materialResourceLight.Get(); }

ID3D12Resource* Light::GetPhongLightResource() const { return materialResourcePhong.Get(); }

ID3D12Resource* Light::GetPointLightResource() const { return materialResourcePoint.Get(); }

ID3D12Resource* Light::GetSpotLightResource() const { return materialResourceSpot.Get(); }

void Light::LightSetting() {

	// Light用のマテリアルリソースを作る
	materialResourceLight = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(DirectionalLight));
	materialResourceLight->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData->intensity = 0.0f;

	// Phong用のマテリアルリソースを作る
	materialResourcePhong = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(CameraForGPU));
	materialResourcePhong->Map(0, nullptr, reinterpret_cast<void**>(&phongLightData));

	phongLightData->worldPosition = { 0.0f, 4.0f, -10.0f };

	// PointLight用のマテリアルリソースを作る
	materialResourcePoint = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(PointLight));
	materialResourcePoint->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData));

	pointLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	pointLightData->position = { 0.0f, 2.0f, 0.0f };
	pointLightData->intensity = 0.0f;
	pointLightData->radius = 3.0f;
	pointLightData->decay = 2.0f;

	// PointLight用のマテリアルリソースを作る
	materialResourceSpot = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(SpotLight));
	materialResourceSpot->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData));

	spotLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	spotLightData->position = { 2.0f, 1.25f, 0.0f };
	spotLightData->distance = 7.0f;
	spotLightData->direction = MyMath::Normalize({ -1.0f, -1.0f, 0.0f });
	spotLightData->intensity = 4.0f;
	spotLightData->decay = 2.0f;
	spotLightData->casAngle = std::cos(std::numbers::pi_v<float> / 3.0f);
	spotLightData->cosFalloffStart = 1.0f;
}

void Light::ImGuiDebug() {

#ifdef _DEBUG

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

#endif // _DEBUG
}