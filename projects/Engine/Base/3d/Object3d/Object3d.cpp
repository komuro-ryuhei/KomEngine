#include "Object3d.h"
#include "Engine/Base/System/System.h"
#include "Engine/lib/Logger/Logger.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif // _DEBUG

void Object3d::Init(BlendType type) {
	Init("object3d", type);
}

void Object3d::Init(const std::string& shaderType, BlendType type) {

	camera_ = defaultCamera_;

	pipelineManager_ = std::make_unique<PipelineManager>();
	pipelineManager_->PSOSetting(shaderType, type);

	transformationMatrixResource =
		KomEngine::System::GetDxCommon()->CreateBufferResource(
			KomEngine::System::GetDxCommon()->GetDevice(),
			sizeof(TransformationMatrix));
	transformationMatrixResource->Map(
		0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));

	transformationMatrixData->WVP = MyMath::MakeIdentity4x4();
	transformationMatrixData->World = MyMath::MakeIdentity4x4();

	objectParamResource_ =
		KomEngine::System::GetDxCommon()->CreateBufferResource(
			KomEngine::System::GetDxCommon()->GetDevice(),
			sizeof(ObjectParams));
	objectParamResource_->Map(
		0, nullptr, reinterpret_cast<void**>(&objectParamData_));

	objectParamData_->useEnvironmentMap = 0;
	objectParamData_->padding_ = { 0.0f, 0.0f, 0.0f };
	objectParamData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };

	transform_ = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f},
	};

	cameraTransform = {
		{1.0f, 1.0f, 1.0f},
		{0.3f, 0.0f, 0.0f},
		{0.0f, 4.0f, -10.0f},
	};
}

void Object3d::Update() {

	// スケール行列（自身のローカルスケール）
	Matrix4x4 scaleMatrix = MyMath::MakeScaleMatrix(transform_.scale);

	// 回転行列
	Matrix4x4 rotateMatrix;
	if (fromBlender_) {
		rotateMatrix = MyMath::MakeRotateMatrixFromBlenderEuler(transform_.rotate);
	}
	else {
		Matrix4x4 rotX = MyMath::MakeRotateXMatrix(transform_.rotate.x);
		Matrix4x4 rotY = MyMath::MakeRotateYMatrix(transform_.rotate.y);
		Matrix4x4 rotZ = MyMath::MakeRotateZMatrix(transform_.rotate.z);
		rotateMatrix = MyMath::Multiply(rotX, MyMath::Multiply(rotY, rotZ));
	}

	// 平行移動行列
	Matrix4x4 translateMatrix = MyMath::MakeTranslateMatrix(transform_.translate);

	// ローカル行列 = R * T
	Matrix4x4 localMatrix = MyMath::Multiply(rotateMatrix, translateMatrix);

	// 親がいるなら：親のスケールを含まないワールド行列と合成
	if (parent_) {
		Matrix4x4 parentMatrix = parent_->GetWorldMatrix();

		// スケール成分を除去（各軸を正規化）
		for (int i = 0; i < 3; ++i) {
			Vector3 axis = { parentMatrix.m[0][i], parentMatrix.m[1][i], parentMatrix.m[2][i] };
			axis = MyMath::Normalize(axis);
			parentMatrix.m[0][i] = axis.x;
			parentMatrix.m[1][i] = axis.y;
			parentMatrix.m[2][i] = axis.z;
		}

		worldMatrix_ = MyMath::Multiply(scaleMatrix, MyMath::Multiply(localMatrix, parentMatrix));
	}
	else {
		worldMatrix_ = MyMath::Multiply(scaleMatrix, localMatrix);
	}

	// ビュー投影行列の生成
	Matrix4x4 projectionMatrix = MyMath::MakePerspectiveFovMatrix(
		0.45f,
		float(winApp_->GetWindowWidth()) / float(winApp_->GetWindowHeight()),
		0.1f,
		100.0f);

	Matrix4x4 worldViewProjectionMatrix;
	if (defaultCamera_) {
		const Matrix4x4& viewProjectionMatrix = defaultCamera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = MyMath::Multiply(worldMatrix_, viewProjectionMatrix);
	}
	else {
		worldViewProjectionMatrix = worldMatrix_;
	}

	// 定数バッファへの書き込み
	transformationMatrixData->WVP = worldViewProjectionMatrix;
	transformationMatrixData->World = worldMatrix_;
	transformationMatrixData->WorldInverseTranspose = MyMath::Transpose4x4(MyMath::Inverse4x4(worldMatrix_));
}


void Object3d::Draw() {

	ComPtr<ID3D12GraphicsCommandList> commandList = KomEngine::System::GetDxCommon()->GetCommandList();

	// コマンド: ルートシグネチャを設定
	commandList->SetGraphicsRootSignature(pipelineManager_->GetRootSignature());

	// コマンド: PSO(Pipeline State Object)を設定
	commandList->SetPipelineState(pipelineManager_->GetGraphicsPipelineState());

	// TransformationMatrixCBufferの場所を設定
	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
	// DirectionalLight の CBV を設定（RootParameter 3）
	commandList->SetGraphicsRootConstantBufferView(3, KomEngine::System::GetLight()->GetLightResource()->GetGPUVirtualAddress());
	//
	commandList->SetGraphicsRootConstantBufferView(4, KomEngine::System::GetLight()->GetPhongLightResource()->GetGPUVirtualAddress());
	//
	commandList->SetGraphicsRootConstantBufferView(5, KomEngine::System::GetLight()->GetPointLightResource()->GetGPUVirtualAddress());

	commandList->SetGraphicsRootConstantBufferView(6, KomEngine::System::GetLight()->GetSpotLightResource()->GetGPUVirtualAddress());

	if (environmentGpuHandle_.ptr != 0) {
		commandList->SetGraphicsRootDescriptorTable(7, environmentGpuHandle_);
	}

	commandList->SetGraphicsRootConstantBufferView(8, objectParamResource_->GetGPUVirtualAddress());

	if (model_) {
		//
		model_->Draw();
	}
}

void Object3d::ImGuiDebug(const char* ImGuiName) {

#ifdef USE_IMGUI

	ImGui::Begin(ImGuiName);

	ImGui::DragFloat3("scale", &transform_.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &transform_.rotate.x, 0.01f);
	ImGui::DragFloat3("translate", &transform_.translate.x, 0.01f);

	ImGui::End();

#endif // _DEBUG
}

void Object3d::SetModel(const std::string& filePath) { model_ = ModelManager::GetInstance()->FindModel(std::move(filePath)); }

void Object3d::SetCamera(Camera* camera) { camera_ = camera; }

void Object3d::SetDefaultCamera(Camera* camera) { defaultCamera_ = camera; }

void Object3d::SetScale(const Vector3& scale) { transform_.scale = scale; }
void Object3d::SetTranslate(const Vector3& translate) { transform_.translate = translate; }
void Object3d::SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }

float Object3d::GetRadius() const { return radius_; }

Vector3 Object3d::GetWorldPosition() const {
	return { worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
}


void Object3d::SetTransform(const Transform& transform) {

	//
	transform_.scale = transform.scale;
	transform_.rotate = transform.rotate;
	transform_.translate = transform.translate;
}

void Object3d::SetColor(const Vector4& color) {
	if (objectParamData_) {
		objectParamData_->color = color;
	}
}

Vector4 Object3d::GetColor() const {
	if (objectParamData_) {
		return objectParamData_->color;
	}
	return { 1.0f, 1.0f, 1.0f, 1.0f };
}

Vector3 Object3d::GetScale() const { return transform_.scale; }
Vector3 Object3d::GetRotate() const { return transform_.rotate; }
Vector3 Object3d::GetTranslate() const { return transform_.translate; }

void Object3d::SetEnvironmentTexture(const std::string& filePath) {
	environmentGpuHandle_ = KomEngine::System::GetTextureManager()->GetSrvHandleGPU(filePath);
	if (objectParamData_) {
		objectParamData_->useEnvironmentMap = 1;
	}
}

void Object3d::SetFromBlender(bool flag) { fromBlender_ = flag; }

Camera* Object3d::GetDefaultCamera() const { return defaultCamera_; }

void Object3d::SetParent(Object3d* parent) {
	parent_ = parent;
}

Object3d* Object3d::GetParent() const {
	return parent_;
}