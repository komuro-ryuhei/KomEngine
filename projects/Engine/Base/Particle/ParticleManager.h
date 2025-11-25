
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

	uint32_t instanceCount = 0;
};

struct SpiralEmitter {
	Vector3 position;
	int count = 0;
	float timer = 0.0f;
	bool active = false;
};
static SpiralEmitter spiralEmitter;

/// <summary>
/// パーティクルマネージャークラス
/// </summary>
class ParticleManager {

public:
	static ParticleManager* GetInstance();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="camera"> カメラ </param>
	/// <param name="type"> ブレンドタイプ </param>
	void Init(Camera* camera, BlendType type);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// 生成
	/// </summary>
	/// <param name="name"> 生成するパーティクルの名前 </param>
	/// <param name="position"> 生成する座標 </param>
	/// <param name="count"> 生成数 </param>
	void Emit(const std::string name, const Vector3& position, uint32_t count);

	void CreateParticleGeoup(const std::string name, const std::string textureFilePath, const std::string& particleType);

	// ランダムで拡散するパーティクル
	Particle MakeRandomParticle(std::mt19937& randomEngine, const Vector3& translate);
	// ヒットエフェクトっぽいパーティクル
	Particle MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate);
	// 埃っぽいパーティクル
	Particle MakeDustParticle(std::mt19937 &randomEngine, const Vector3 &translate);
	// マズルフラッシュのパーティクル
	Particle MakeMuzzleFlashParticle(std::mt19937& randomEngine, const Vector3& translate);

	// 白いリングのエフェクト
	Particle MakeRingParticle(std::mt19937& randomEngine, const Vector3& translate);
	// シリンダーのエフェクト
	Particle MakeCylinderParticle(std::mt19937& randomEngine, const Vector3& translate);
	// 月のような光のエフェクト
	Particle MakeMoonLightParticle(const Vector3& translate, bool isVertical);
	// 渦巻くエフェクト
	Particle MakeSpiralParticle(const Vector3& translate, float angleOffset);

	/// <summary>
	/// 存在確認
	/// </summary>
	/// <param name="name"> 名前 </param>
	bool Exists(const std::string& name) const;

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

	/// <summary>
	///　頂点データ作成
	/// </summary>
	/// <param name="group">　グループ　</param>
	/// <param name="particleType"> パーティクルのタイプ </param>
	void MakeVertexData(ParticleGroup& group, const std::string& particleType);

	/// <summary>
	/// 渦巻きエミッター更新
	/// </summary>
	void UpdateSpiralEmitter();
};