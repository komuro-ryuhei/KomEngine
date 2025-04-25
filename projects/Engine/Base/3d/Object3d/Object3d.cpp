#include "Object3d.h"
#include "Engine/Base/System/System.h"
#include "Engine/lib/Logger/Logger.h"
#include "externals/imgui/imgui.h"

void Object3d::Init() {

	camera_ = defaultCamera_;

	//
	pipelineManager_ = std::make_unique<PipelineManager>();
	pipelineManager_->PSOSetting("object3d");

	// 座標変換用
	transformationMatrixResource = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(TransformationMatrix));
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	// 単位行列を書き込む
	transformationMatrixData->WVP = MyMath::MakeIdentity4x4();
	transformationMatrixData->World = MyMath::MakeIdentity4x4();

	transform_ = {
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

	Matrix4x4 worldMatrix = MyMath::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
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

	if (model_) {
		//
		model_->Draw();
	}
}

void Object3d::ImGuiDebug() {

	ImGui::Begin("object3d");

	ImGui::DragFloat3("scale", &transform_.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &transform_.rotate.x, 0.01f);
	ImGui::DragFloat3("translate", &transform_.translate.x, 0.01f);

	ImGui::End();
}

void Object3d::SetModel(const std::string& filePath) { model_ = ModelManager::GetInstance()->FindModel(std::move(filePath)); }

void Object3d::SetCamera(Camera* camera) { camera_ = camera; }

void Object3d::SetDefaultCamera(Camera* camera) { defaultCamera_ = camera; }

void Object3d::SetScale(const Vector3& scale) { transform_.scale = scale; }
void Object3d::SetTranslate(const Vector3& translate) { transform_.translate = translate; }
void Object3d::SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }

void Object3d::SetTransform(const Transform& transform) {
	
	// 
	transform_.scale = transform.scale;
	transform_.rotate = transform.rotate;
	transform_.translate = transform.translate;
}

Vector3 Object3d::GetScale() const { return transform_.scale; }
Vector3 Object3d::GetRotate() const { return transform_.rotate; }
Vector3 Object3d::GetTranslate() const { return transform_.translate; }

Camera* Object3d::GetDefaultCamera() const { return defaultCamera_; }