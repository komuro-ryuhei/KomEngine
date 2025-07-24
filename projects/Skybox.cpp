#include "Skybox.h"
#include "Engine/Base/System/System.h"

#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif // _DEBUG

void Skybox::SetDefaultCamera(Camera* camera) { defaultCamera_ = camera; }

constexpr uint32_t kSkyboxVertexCount = 36;

void Skybox::Init(const std::string& filename) {

	filename_ = filename;

	//
	pipelineManager_ = std::make_unique<PipelineManager>();
	pipelineManager_->PSOSetting("skybox", BlendType::BLEND_NONE);

	vertexResource = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(VertexData) * kSkyboxVertexCount);
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * kSkyboxVertexCount);
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	materialResource = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(Material));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = false;
	materialData->uvTransform = MyMath::MakeIdentity4x4();
	materialData->shininess = 48.3f;

	TextureManager::GetInstance()->LoadTexture(std::move(filename));

	// 座標変換用
	transformationMatrixResource = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(TransformationMatrix));
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	// 単位行列を書き込む
	transformationMatrixData->WVP = MyMath::MakeIdentity4x4();
	transformationMatrixData->World = MyMath::MakeIdentity4x4();

	transform = {
		{100.0f, 100.0f, 100.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f},
	};

	InitPosition();
}

void Skybox::Update() {

	// カメラが存在していれば更新
	if (defaultCamera_) {
		// カメラの位置にスカイボックスを追従
		transform.translate = defaultCamera_->GetTranaslate();

		// ワールド行列（スケールと回転）
		Matrix4x4 worldMatrix = MyMath::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

		// カメラのビュー＆プロジェクション行列を取得
		Matrix4x4 viewMatrix = defaultCamera_->GetViewMatrix();
		Matrix4x4 projectionMatrix = defaultCamera_->GetProjectionMatrix();

		// ビュー行列の位置成分を無効化（回転のみ反映）
		viewMatrix.m[3][0] = 0.0f;
		viewMatrix.m[3][1] = 0.0f;
		viewMatrix.m[3][2] = 0.0f;

		Matrix4x4 viewProjMatrix = MyMath::Multiply(viewMatrix, projectionMatrix);
		Matrix4x4 worldViewProjectionMatrix = MyMath::Multiply(worldMatrix, viewProjMatrix);

		// バッファ転送
		transformationMatrixData->WVP = worldViewProjectionMatrix;
		transformationMatrixData->World = worldMatrix;
		transformationMatrixData->WorldInverseTranspose = MyMath::Transpose4x4(MyMath::Inverse4x4(worldMatrix));
	}
}

void Skybox::Draw() {

	ComPtr<ID3D12GraphicsCommandList> commandList = System::GetDxCommon()->GetCommandList();

	// コマンド: ルートシグネチャを設定
	commandList->SetGraphicsRootSignature(pipelineManager_->GetRootSignature());
	// コマンド: PSO(Pipeline State Object)を設定
	commandList->SetPipelineState(pipelineManager_->GetGraphicsPipelineState());

	commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	// TransformationMatrixCBufferの場所を設定
	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());

	commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(filename_));

	// Modelの描画
	commandList->DrawInstanced(kSkyboxVertexCount, 1, 0, 0);
}

void Skybox::InitPosition() {

	// +X（右）
	vertexData[0].position = { 1.0f, 1.0f, -1.0f, 1.0f };
	vertexData[1].position = { 1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[2].position = { 1.0f, -1.0f, 1.0f, 1.0f };
	vertexData[3].position = { 1.0f, 1.0f, -1.0f, 1.0f };
	vertexData[4].position = { 1.0f, -1.0f, 1.0f, 1.0f };
	vertexData[5].position = { 1.0f, 1.0f, 1.0f, 1.0f };

	// -X（左）
	vertexData[6].position = { -1.0f, 1.0f, 1.0f, 1.0f };
	vertexData[7].position = { -1.0f, -1.0f, 1.0f, 1.0f };
	vertexData[8].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[9].position = { -1.0f, 1.0f, 1.0f, 1.0f };
	vertexData[10].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[11].position = { -1.0f, 1.0f, -1.0f, 1.0f };

	// +Y（上）
	vertexData[12].position = { -1.0f, 1.0f, -1.0f, 1.0f };
	vertexData[13].position = { -1.0f, 1.0f, 1.0f, 1.0f };
	vertexData[14].position = { 1.0f, 1.0f, 1.0f, 1.0f };
	vertexData[15].position = { -1.0f, 1.0f, -1.0f, 1.0f };
	vertexData[16].position = { 1.0f, 1.0f, 1.0f, 1.0f };
	vertexData[17].position = { 1.0f, 1.0f, -1.0f, 1.0f };

	// -Y（下）
	vertexData[18].position = { -1.0f, -1.0f, 1.0f, 1.0f };
	vertexData[19].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[20].position = { 1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[21].position = { -1.0f, -1.0f, 1.0f, 1.0f };
	vertexData[22].position = { 1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[23].position = { 1.0f, -1.0f, 1.0f, 1.0f };

	// +Z（前）
	vertexData[24].position = { 1.0f, 1.0f, 1.0f, 1.0f };
	vertexData[25].position = { 1.0f, -1.0f, 1.0f, 1.0f };
	vertexData[26].position = { -1.0f, -1.0f, 1.0f, 1.0f };
	vertexData[27].position = { 1.0f, 1.0f, 1.0f, 1.0f };
	vertexData[28].position = { -1.0f, -1.0f, 1.0f, 1.0f };
	vertexData[29].position = { -1.0f, 1.0f, 1.0f, 1.0f };

	// -Z（後）
	vertexData[30].position = { -1.0f, 1.0f, -1.0f, 1.0f };
	vertexData[31].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[32].position = { 1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[33].position = { -1.0f, 1.0f, -1.0f, 1.0f };
	vertexData[34].position = { 1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[35].position = { 1.0f, 1.0f, -1.0f, 1.0f };
}

void Skybox::ImGuiDebug() {

#ifdef _DEBUG

	ImGui::Begin("Skybox");
	
	ImGui::SliderFloat3("Scale", &transform.scale.x, 0.0f, 100.0f);
	ImGui::SliderAngle("Rotate X", &transform.rotate.x);
	ImGui::SliderAngle("Rotate Y", &transform.rotate.y);
	ImGui::SliderAngle("Rotate Z", &transform.rotate.z);
	ImGui::DragFloat3("Translate", &transform.translate.x, 0.01f);
	
	ImGui::End();

#endif // _DEBUG
}