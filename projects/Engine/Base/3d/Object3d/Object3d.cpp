#include "Object3d.h"
#include "Engine/Base/System/System.h"
#include "Engine/lib/Logger/Logger.h"

#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif // _DEBUG

void Object3d::Init(BlendType type) {

	camera_ = defaultCamera_;

	//
	pipelineManager_ = std::make_unique<PipelineManager>();
	pipelineManager_->PSOSetting("object3d", type);

	// 座標変換用
	transformationMatrixResource = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(TransformationMatrix));
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	// 単位行列を書き込む
	transformationMatrixData->WVP = MyMath::MakeIdentity4x4();
	transformationMatrixData->World = MyMath::MakeIdentity4x4();

	// 
	objectParamResource_ = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(ObjectParams));
	objectParamResource_->Map(0, nullptr, reinterpret_cast<void**>(&objectParamData_));
	objectParamData_->useEnvironmentMap = false;

	transform = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f},
	};

	cameraTransform = {
		{1.0f, 1.0f, 1.0f  },
		{0.3f, 0.0f, 0.0f  },
		{0.0f, 4.0f, -10.0f},
	};
}

void Object3d::Update() {

	Matrix4x4 worldMatrix = MyMath::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 projectionMatrix = MyMath::MakePerspectiveFovMatrix(0.45f, float(winApp_->GetWindowWidth()) / float(winApp_->GetWindowHeight()), 0.1f, 100.0f);
	Matrix4x4 worldViewProjectionMatrix;
	if (defaultCamera_) {
		const Matrix4x4& viewProjectionMatrix = defaultCamera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = MyMath::Multiply(worldMatrix, viewProjectionMatrix);
	} else {
		worldViewProjectionMatrix = worldMatrix;
	}
	transformationMatrixData->WVP = worldViewProjectionMatrix;
	transformationMatrixData->World = worldMatrix;
	transformationMatrixData->WorldInverseTranspose = MyMath::Inverse4x4(worldMatrix);
	transformationMatrixData->WorldInverseTranspose = MyMath::Transpose4x4(transformationMatrixData->WorldInverseTranspose);
}

void Object3d::Draw() {

	ComPtr<ID3D12GraphicsCommandList> commandList = System::GetDxCommon()->GetCommandList();

	// コマンド: ルートシグネチャを設定
	commandList->SetGraphicsRootSignature(pipelineManager_->GetRootSignature());

	// コマンド: PSO(Pipeline State Object)を設定
	commandList->SetPipelineState(pipelineManager_->GetGraphicsPipelineState());

	// TransformationMatrixCBufferの場所を設定
	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
	// DirectionalLight の CBV を設定（RootParameter 3）
	commandList->SetGraphicsRootConstantBufferView(3, System::GetMesh()->GetLightResource()->GetGPUVirtualAddress());
	//
	commandList->SetGraphicsRootConstantBufferView(4, System::GetMesh()->GetPhongLightResource()->GetGPUVirtualAddress());
	//
	commandList->SetGraphicsRootConstantBufferView(5, System::GetMesh()->GetPointLightResource()->GetGPUVirtualAddress());

	commandList->SetGraphicsRootConstantBufferView(6, System::GetMesh()->GetSpotLightResource()->GetGPUVirtualAddress());

	if (environmentGpuHandle_.ptr != 0) {
		commandList->SetGraphicsRootDescriptorTable(7, environmentGpuHandle_);
	}

	commandList->SetGraphicsRootConstantBufferView(8, objectParamResource_->GetGPUVirtualAddress());

	if (model_) {
		//
		model_->Draw();
	}
}

void Object3d::ImGuiDebug() {

#ifdef _DEBUG

	ImGui::Begin("object3d");

	ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
	ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);

	ImGui::End();

#endif // _DEBUG
}

void Object3d::SetModel(const std::string& filePath) { model_ = ModelManager::GetInstance()->FindModel(std::move(filePath)); }

void Object3d::SetScale(const Vector3& scale) { transform.scale = scale; }

void Object3d::SetRotate(const Vector3& rotate) { transform.rotate = rotate; }

void Object3d::SetCamera(Camera* camera) { camera_ = camera; }

void Object3d::SetTranslate(const Vector3& translate) { transform.translate = translate; }

void Object3d::SetDefaultCamera(Camera* camera) { defaultCamera_ = camera; }

void Object3d::SetEnvironmentTexture(const std::string& filePath) {
	environmentGpuHandle_ = TextureManager::GetInstance()->GetSrvHandleGPU(filePath);
	if (objectParamData_) {
		objectParamData_->useEnvironmentMap = true;
	}
}


Camera* Object3d::GetDefaultCamera() const { return defaultCamera_; }