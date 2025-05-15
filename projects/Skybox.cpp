#include "Skybox.h"
#include "Engine/Base/System/System.h"

constexpr uint32_t kSkyboxVertexCount = 24;

void Skybox::Init(DirectXCommon* dxCommon, const std::string& directoryPath, const std::string& filename) {

	dxCommon_ = dxCommon;
	filename_ = filename;

	//
	pipelineManager_ = std::make_unique<PipelineManager>();
	pipelineManager_->PSOSetting("object3d", BlendType::BLEND_NONE);

	vertexResource = dxCommon_->CreateBufferResource(dxCommon_->GetDevice(), sizeof(VertexData));
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData));
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	materialResource = dxCommon_->CreateBufferResource(dxCommon_->GetDevice(), sizeof(Material));
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
	    {1.0f, 1.0f, 1.0f},
	    {0.0f, 0.0f, 0.0f},
	    {0.0f, 0.0f, 0.0f},
	};

	cameraTransform = {
	    {1.0f, 1.0f, 1.0f  },
	    {0.3f, 0.0f, 0.0f  },
	    {0.0f, 4.0f, -10.0f},
	};

	InitPosition();
}

void Skybox::Update() {

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

void Skybox::Draw() {

	ComPtr<ID3D12GraphicsCommandList> commandList = dxCommon_->GetCommandList();

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

	// 右面
	vertexData[0].position = {1.0f, 1.0f, 1.0f, 1.0f};
	vertexData[1].position = {1.0f, 1.0f, -1.0f, 1.0f};
	vertexData[2].position = {1.0f, -1.0f, 1.0f, 1.0f};
	vertexData[3].position = {1.0f, -1.0f, -1.0f, 1.0f};
	// 左面
	vertexData[4].position = {-1.0f, 1.0f, -1.0f, 1.0f};
	vertexData[5].position = {-1.0f, 1.0f, 1.0f, 1.0f};
	vertexData[6].position = {-1.0f, -1.0f, -1.0f, 1.0f};
	vertexData[7].position = {-1.0f, -1.0f, 1.0f, 1.0f};
	// 前面
	vertexData[8].position = {-1.0f, 1.0f, 1.0f, 1.0f};
	vertexData[9].position = {1.0f, 1.0f, 1.0f, 1.0f};
	vertexData[10].position = {-1.0f, -1.0f, 1.0f, 1.0f};
	vertexData[11].position = {1.0f, -1.0f, 1.0f, 1.0f};
	// 後面
	vertexData[12].position = {1.0f, 1.0f, -1.0f, 1.0f};
	vertexData[13].position = {-1.0f, 1.0f, -1.0f, 1.0f};
	vertexData[14].position = {1.0f, -1.0f, -1.0f, 1.0f};
	vertexData[15].position = {-1.0f, -1.0f, -1.0f, 1.0f};
	// 上面
	vertexData[16].position = {1.0f, 1.0f, -1.0f, 1.0f};
	vertexData[17].position = {-1.0f, 1.0f, -1.0f, 1.0f};
	vertexData[18].position = {1.0f, 1.0f, 1.0f, 1.0f};
	vertexData[19].position = {-1.0f, 1.0f, 1.0f, 1.0f};
	// 下面
	vertexData[20].position = {1.0f, -1.0f, 1.0f, 1.0f};
	vertexData[21].position = {-1.0f, -1.0f, 1.0f, 1.0f};
	vertexData[22].position = {1.0f, -1.0f, -1.0f, 1.0f};
	vertexData[23].position = {-1.0f, -1.0f, -1.0f, 1.0f};
}