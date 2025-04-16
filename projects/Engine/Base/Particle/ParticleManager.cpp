#include "ParticleManager.h"

#include "Engine/Base/System/System.h"
#include "Engine/lib/Math/MyMath.h"

#include <numbers>

ParticleManager* ParticleManager::instance = nullptr;

ParticleManager* ParticleManager::GetInstance() {

	if (instance == nullptr) {
		instance = new ParticleManager;
	}
	return instance;
}

void ParticleManager::Init(Camera* camera) {

	//
	camera_ = camera;

	//
	pipelineManager_ = std::make_unique<PipelineManager>();
	pipelineManager_->PSOSetting("particle");

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());
	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);

	// 頂点データの初期化

	// 座標
	modelData.vertices.push_back({
	    .position = {1.0f, 1.0f, 0.0f, 1.0f},
          .texcoord = {0.0f, 0.0f},
          .normal = {0.0f, 0.0f, 1.0f}
    });
	modelData.vertices.push_back({
	    .position = {-1.0f, 1.0f, 0.0f, 1.0f},
          .texcoord = {1.0f, 0.0f},
          .normal = {0.0f, 0.0f, 1.0f}
    });
	modelData.vertices.push_back({
	    .position = {1.0f, -1.0f, 0.0f, 1.0f},
          .texcoord = {0.0f, 1.0f},
          .normal = {0.0f, 0.0f, 1.0f}
    });
	modelData.vertices.push_back({
	    .position = {1.0f, -1.0f, 0.0f, 1.0f},
          .texcoord = {0.0f, 1.0f},
          .normal = {0.0f, 0.0f, 1.0f}
    });
	modelData.vertices.push_back({
	    .position = {-1.0f, 1.0f, 0.0f, 1.0f},
          .texcoord = {1.0f, 0.0f},
          .normal = {0.0f, 0.0f, 1.0f}
    });
	modelData.vertices.push_back({
	    .position = {-1.0f, -1.0f, 0.0f, 1.0f},
          .texcoord = {1.0f, 1.0f},
          .normal = {0.0f, 0.0f, 1.0f}
    });

	// 頂点リソースの作成
	vertexResource = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(VertexData) * modelData.vertices.size());

	// VBVの作成
	// リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	// 1頂点当あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// 頂点リソースに頂点データを書き込む
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	// 頂点データにリソースをコピー
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

void ParticleManager::Update() {

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	Matrix4x4 viewMatrix = camera_->GetViewMatrix();
	Matrix4x4 projectionMatrix = camera_->GetProjectionMatrix();

	for (auto& [name, group] : particleGroups) {

		size_t numInstance = 0;

		for (auto it = group.particles.begin(); it != group.particles.end();) {
			Particle& particle = *it;

			// 時間経過
			particle.currentTime += 1.0f / 60.0f;
			if (particle.currentTime >= particle.lifeTime) {
				it = group.particles.erase(it); // 寿命が尽きたパーティクルを削除
				continue;
			}

			// アルファ値を寿命に応じて減衰
			float lifeRatio = 1.0f - (particle.currentTime / particle.lifeTime);
			lifeRatio = std::clamp(lifeRatio, 0.0f, 1.0f);
			particle.color.w = lifeRatio;

			// 速度による移動
			particle.transform.translate.x += particle.velocity.x;
			particle.transform.translate.y += particle.velocity.y;
			particle.transform.translate.z += particle.velocity.z;

			// GPUバッファの最大数 (kInstanceNum) を超えないようにする
			if (numInstance < group.kInstanceNum) {

				Matrix4x4 worldMatrix = MyMath::MakeAffineMatrix(particle.transform.scale, particle.transform.rotate, particle.transform.translate);
				group.instancingData[numInstance].World = worldMatrix;
				group.instancingData[numInstance].WVP = MyMath::Multiply(MyMath::Multiply(worldMatrix, viewMatrix), projectionMatrix);
				group.instancingData[numInstance].color = Vector4(particle.color);
				++numInstance;
			}

			++it;
		}
	}
}

void ParticleManager::Draw() {

	// コマンド: ルートシグネチャを設定
	System::GetDxCommon()->GetCommandList()->SetGraphicsRootSignature(pipelineManager_->GetRootSignature());

	// コマンド: PSO(Pipeline State Object)を設定
	System::GetDxCommon()->GetCommandList()->SetPipelineState(pipelineManager_->GetGraphicsPipelineState());

	// コマンド: プリミティブトポロジーを設定 (三角形リスト)
	System::GetDxCommon()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// コマンド: VBV(Vertex Buffer View)を設定
	System::GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	// 全てのパーティクルグループについて処理する
	for (auto& [name, group] : particleGroups) {

		// コマンド: インスタンシングデータのSRVをDescriptorTableに設定
		System::GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(0, System::GetSrvManager()->GetGPUDescriptorHandle(group.instancingSrvIndex));

		// コマンド: テクスチャのSRVをDescriptorTableに設定
		System::GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(1, System::GetSrvManager()->GetGPUDescriptorHandle(group.srvIndex));

		// コマンド: DrawCall (インスタンシング描画)
		System::GetDxCommon()->GetCommandList()->DrawInstanced(6, static_cast<UINT>(group.particles.size()), 0, 0);
	}
}

void ParticleManager::Finalize() {

	delete instance;
	instance = nullptr;
}

void ParticleManager::Emit(const std::string name, const Vector3& position, uint32_t count) {
	// 指定したパーティクルグループが存在するか確認
	auto it = particleGroups.find(name);
	if (it == particleGroups.end()) {
		assert(false && "Particle group not found.");
		return;
	}

	ParticleGroup& group = it->second;

	// ランダムエンジンの初期化
	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	// 指定した数だけパーティクルを発生
	for (uint32_t i = 0; i < count; ++i) {
		// group.particles.push_back(MakeRandomParticle(randomEngine, position));
		group.particles.push_back(MakeNewParticle(randomEngine, position));
	}
}

void ParticleManager::CreateParticleGeoup(const std::string name, const std::string textureFilePath) {

	// 登録済みかチェック
	assert(particleGroups.find(name) == particleGroups.end());

	ParticleGroup newParticle;
	newParticle.materialData.textureFilePath = textureFilePath;

	TextureManager::GetInstance()->LoadTexture(textureFilePath);

	uint32_t srvIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
	newParticle.srvIndex = srvIndex;

	newParticle.kInstanceNum = 0xffff;

	newParticle.instancingResource = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(ParticleForGPU) * newParticle.kInstanceNum);
	newParticle.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&newParticle.instancingData));

	newParticle.instancingSrvIndex = System::GetSrvManager()->Allocate();

	System::GetSrvManager()->CreateSRVforStructuredBuffer(newParticle.instancingSrvIndex, newParticle.instancingResource.Get(), newParticle.kInstanceNum, sizeof(ParticleForGPU));

	particleGroups[name] = newParticle;

	// newParticle.srvIndex = instancingSrvIndex;
}

// ランダムなパーティクル生成関数
Particle ParticleManager::MakeRandomParticle(std::mt19937& randomEngine, const Vector3& translate) {

	std::uniform_real_distribution<float> distribution(-0.5f, 0.5f);
	std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
	std::uniform_real_distribution<float> distTime(2.0f, 4.0f);

	Particle particle;
	Vector3 randomTranslate{distribution(randomEngine), distribution(randomEngine), distribution(randomEngine)};
	particle.transform.scale = {1.0f, 1.0f, 1.0f};
	particle.transform.rotate = {0.0f, 0.0f, 0.0f};
	particle.transform.translate = translate + randomTranslate;
	particle.velocity = {distribution(randomEngine), distribution(randomEngine), distribution(randomEngine)};
	particle.color = {distColor(randomEngine), distColor(randomEngine), distColor(randomEngine), 1.0f};
	particle.lifeTime = distTime(randomEngine);
	particle.currentTime = 0.0f;

	return particle;
}

Particle ParticleManager::MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate) {

	std::uniform_real_distribution<float> distScale(0.4f, 1.5f);
	std::uniform_real_distribution<float> distRotate(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);

	// 
	Particle particle;
	particle.transform.scale = {0.05f, distScale(randomEngine), 1.0f};
	particle.transform.rotate = {0.0f, 0.0f, distRotate(randomEngine)};
	particle.transform.translate = translate;
	particle.velocity = {0.0f, 0.0f, 0.0f};
	particle.color = {1.0f, 1.0f, 1.0f, 1.0f};
	particle.lifeTime = 1.0f;
	particle.currentTime = 0.0f;
	return particle;
}