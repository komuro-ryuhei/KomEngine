
#pragma once

// C++
#include <Random>

// MyClass
#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/PSO/PipelineManager/PipelineManager.h"
#include "Engine/Base/SrvManager/SrvManager.h"
#include "Engine/Base/TextureManager/TextureManager.h"
#include "Engine/lib/ComPtr/ComPtr.h"
#include "struct.h"

struct Particle {
	Transform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
};

struct ParticleForGPU {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 color;
};

struct ParticleGroup {
	MaterialData materialData;
	std::list<Particle> particles;
	uint32_t srvIndex;
	uint32_t instancingSrvIndex;
	ComPtr<ID3D12Resource> instancingResource;
	uint32_t kInstanceNum;
	ParticleForGPU* instancingData;
	std::vector<VertexData> vertices;
	ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
};

class ParticleManager {

public:
	static ParticleManager* GetInstance();

	void Init(Camera* camera, BlendType type);

	void Update();

	void Draw();

	void Finalize();

	void Emit(const std::string name, const Vector3& position, uint32_t count);

	void CreateParticleGeoup(const std::string name, const std::string textureFilePath, const std::string& particleType);

	// ランダムで拡散するパーティクル
	Particle MakeRandomParticle(std::mt19937& randomEngine, const Vector3& translate);
	// ヒットエフェクトっぽいパーティクル
	Particle MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate);
	// 白いリングのエフェクト
	Particle MakeRingParticle(std::mt19937& randomEngine, const Vector3& translate);
	// シリンダーのエフェクト
	Particle MakeCylinderParticle(std::mt19937& randomEngine, const Vector3& translate);

private:
	ParticleManager() = default;
	~ParticleManager() = default;
	ParticleManager(ParticleManager&) = delete;
	ParticleManager& operator=(ParticleManager&) = delete;

public:
	static ParticleManager* instance;

private:
	Camera* camera_ = nullptr;

	ModelData modelData;

	VertexData* vertexData = nullptr;

	ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	std::unordered_map<std::string, ParticleGroup> particleGroups;

	// 
	std::unique_ptr<PipelineManager> pipelineManager_ = nullptr;

private:

	void MakeVertexData(ParticleGroup& group, const std::string& particleType);
};