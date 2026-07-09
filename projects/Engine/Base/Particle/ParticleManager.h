#pragma once

// C++
#include <Random>
#include <functional>
#include <fstream>
#include <filesystem>
#include "externals/nlohmann/json.hpp"

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


struct RangeF {
	float min = 0.0f;
	float max = 0.0f;
};

enum class ParticleBehaviorType {
	Default,
	Explosion,
	Hit,
	Dust,
	Ring,
	MissileFlame,
	ChargeCore,
	ChargePulse,
	PlayerChargeLine,
	ChargeAura
};

struct ParticlePreset {
	std::string name;

	std::string textureFilePath;
	std::string meshType;        // "a", "ring", "cylinder" みたいなやつ
	ParticleBehaviorType behaviorType = ParticleBehaviorType::Default;

	RangeF lifeTime;
	RangeF speed;
	RangeF scale;
	RangeF angle;
	RangeF velocityY;

	Vector4 colorMin{ 1,1,1,1 };
	Vector4 colorMax{ 1,1,1,1 };

	bool billboard = true;
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

	ParticleManager() = default;
	~ParticleManager() = default;
	ParticleManager(ParticleManager&) = delete;
	ParticleManager& operator=(ParticleManager&) = delete;

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="camera"> カメラ </param>
	/// <param name="type"> ブレンドタイプ </param>
	void Init(BlendType type);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	// 発生中のパーティクルを全削除
	void ClearParticles();

public:

	// setter
	void SetCamera(Camera* camera) { camera_ = camera; }

	/// <summary>
	/// 生成
	/// </summary>
	/// <param name="name"> 生成するパーティクルの名前 </param>
	/// <param name="position"> 生成する座標 </param>
	/// <param name="count"> 生成数 </param>
	void Emit(const std::string& name, const Vector3& position, uint32_t count);

	void CreateParticleGroup(const std::string& name, const std::string& textureFilePath, const std::string& particleType);
	[[deprecated("Use CreateParticleGroup instead.")]]
	void CreateParticleGeoup(const std::string& name, const std::string& textureFilePath, const std::string& particleType);

	// プリセットからパーティクルグループを作成して登録
	bool CreateParticleGroupFromPreset(const std::string& presetName);

	// プリセットの保存と読み込み
	bool SavePresetToJson(const std::string& name, const std::string& filePath) const;
	bool LoadPresetFromJson(const std::string& filePath);
	bool LoadPresetFromJson(const std::string& filePath, std::string* outLoadedPresetName);

	// ランダムで拡散するパーティクル
	Particle MakeRandomParticle(std::mt19937& randomEngine, const Vector3& translate);
	// ヒットエフェクトっぽいパーティクル
	Particle MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate);
	// 埃っぽいパーティクル
	Particle MakeDustParticle(std::mt19937& randomEngine, const Vector3& translate);
	// マズルフラッシュのパーティクル
	Particle MakeMuzzleFlashParticle(std::mt19937& randomEngine, const Vector3& translate);

	// 弾道パーティクル（細い白線）
	Particle MakeTrailParticle(const Vector3& pos);

	// 白いリングのエフェクト
	Particle MakeRingParticle(std::mt19937& randomEngine, const Vector3& translate);
	// シリンダーのエフェクト
	Particle MakeCylinderParticle(std::mt19937& randomEngine, const Vector3& translate);
	// 月のような光のエフェクト
	Particle MakeMoonLightParticle(const Vector3& translate, bool isVertical);
	// 渦巻くエフェクト
	Particle MakeSpiralParticle(const Vector3& translate, float angleOffset);

	// 敵のチャージ中の「中心に吸い込まれる」パーティクル
	Particle MakeChargeCoreParticle(std::mt19937& randomEngine, const Vector3& center);
	// 敵のチャージ中の「脈動リング」
	Particle MakeChargePulseRingParticle(std::mt19937& randomEngine, const Vector3& center);

	// チャージのラインエフェクト
	Particle MakePlayerChargeLineParticle(std::mt19937& randomEngine, const Vector3& center);

	// チャージ中の「気の塊本体」
	Particle MakeChargeAuraParticle(std::mt19937& randomEngine, const Vector3& center);

	// missile flame
	Particle MakeMissileFlameParticle(std::mt19937& randomEngine, const Vector3& pos, const Vector3& forward);

	// 腕の風切り
	Particle MakeArmWindSlashParticle(std::mt19937& randomEngine, const Vector3& pos, const Vector3& forward);

	// 爆発エフェクトのパーティクル
	Particle MakeExplosionParticle(std::mt19937& randomEngine, const Vector3& translate);

	// 弾道専用 Emit
	void EmitTrail(const Vector3& pos, const Vector3& dir);

	// ミサイルの炎専用 Emit
	void EmitMissileFlame(const Vector3& pos, const Vector3& forward, uint32_t count);

	// // 腕攻撃の風切り Emit
	void EmitArmWindSlash(const Vector3& pos, const Vector3& forward, uint32_t count);

	/// <summary>
	/// 存在確認
	/// </summary>
	/// <param name="name"> 名前 </param>
	bool Exists(const std::string& name) const;

	void SetChargeEffectColor(const Vector4& core, const Vector4& pulse) {
		chargeCoreColor_ = core;
		chargePulseColor_ = pulse;
	}

	// プリセット関連
	bool HasPreset(const std::string& name) const;
	ParticlePreset* FindPreset(const std::string& name);
	const ParticlePreset* FindPreset(const std::string& name) const;

	void RegisterPreset(const ParticlePreset& preset);
	bool RemovePreset(const std::string& name);

	const std::unordered_map<std::string, ParticlePreset>& GetPresets() const { return presets_; }

private:

	Camera* camera_ = nullptr;

	ModelData modelData;

	VertexData* vertexData = nullptr;

	ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	std::unordered_map<std::string, ParticleGroup> particleGroups;

	// 
	std::unique_ptr<PipelineManager> pipelineManager_ = nullptr;

	// プリセットデータ
	std::unordered_map<std::string, ParticlePreset> presets_;

	// チャージエフェクトの色
	Vector4 chargeCoreColor_{ 0.65f, 0.90f, 1.00f, 1.0f };
	Vector4 chargePulseColor_{ 0.75f, 0.90f, 1.00f, 1.0f };

private:

	/// <summary>
	///　頂点データ作成
	/// </summary>
	/// <param name="group">　グループ　</param>
	/// <param name="particleType"> パーティクルのタイプ </param>
	void MakeVertexData(ParticleGroup& group, const std::string& particleType);

	// デフォルトのプリセット登録
	void RegisterDefaultPresets();

	// プリセットからパーティクルを作成
	Particle MakeParticleFromPreset(std::mt19937& randomEngine, const ParticlePreset& preset, const Vector3& translate);

	/// <summary>
	/// 渦巻きエミッター更新
	/// </summary>
	void UpdateSpiralEmitter();

	// 
	using EmitFunc = std::function<void(ParticleGroup&, const Vector3&, uint32_t)>;
	std::unordered_map<std::string, EmitFunc> emitTable_;

	void BuildEmitTable();

	void EmitExplosion(ParticleGroup& group, const Vector3& position, uint32_t count);
	void EmitHit(ParticleGroup& group, const Vector3& position, uint32_t count);
	void EmitMuzzle(ParticleGroup& group, const Vector3& position, uint32_t count);
	void EmitDust(ParticleGroup& group, const Vector3& position, uint32_t count);
	void EmitRing(ParticleGroup& group, const Vector3& position, uint32_t count);
	void EmitCylinder(ParticleGroup& group, const Vector3& position, uint32_t count);
	void EmitMoonLight(ParticleGroup& group, const Vector3& position, uint32_t count);
	void EmitChargeCore(ParticleGroup& group, const Vector3& position, uint32_t count);
	void EmitChargePulse(ParticleGroup& group, const Vector3& position, uint32_t count);
	void EmitRibbon(ParticleGroup& group, const Vector3& position, uint32_t count);
	void EmitTrailGroup(ParticleGroup& group, const Vector3& position, uint32_t count);
	void EmitPlayerChargeLine(ParticleGroup& group, const Vector3& position, uint32_t count);
	void EmitChargeAura(ParticleGroup& group, const Vector3& position, uint32_t count);
};
