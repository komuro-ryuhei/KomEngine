#include "ParticleManager.h"

#include "Engine/Base/System/System.h"
#include "Engine/lib/Math/MyMath.h"

#include <numbers>

void ParticleManager::Init(BlendType type) {

	//
	pipelineManager_ = std::make_unique<PipelineManager>();
	pipelineManager_->PSOSetting("particle", type);

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());
	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
}

void ParticleManager::Update() {

	if (!camera_) {
		return;
	}

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	Matrix4x4 viewMatrix = camera_->GetViewMatrix();
	Matrix4x4 projectionMatrix = camera_->GetProjectionMatrix();

	UpdateSpiralEmitter();

	// 今の実装は固定60fps進行なので、それを変えない
	const float dt = 1.0f / 60.0f;

	for (auto& [name, group] : particleGroups) {

		size_t numInstance = 0;

		for (auto it = group.particles.begin(); it != group.particles.end();) {
			Particle& particle = *it;

			// 時間経過
			particle.currentTime += dt;
			if (particle.currentTime >= particle.lifeTime) {
				it = group.particles.erase(it);
				continue;
			}

			// 寿命比
			float t = particle.currentTime / particle.lifeTime;  // 0 -> 1
			t = std::clamp(t, 0.0f, 1.0f);

			// アルファ値を寿命に応じて減衰
			float lifeRatio = 1.0f - t;
			lifeRatio = std::clamp(lifeRatio, 0.0f, 1.0f);
			particle.color.w = lifeRatio;

			// -----------------------------
			// チャージ専用の見た目調整
			// -----------------------------
			if (name == "charge_core") {
				// 少し回転（エネルギー感）
				particle.transform.rotate.z += 0.25f;

				// 寿命で小さくなる（最後は消える前に極小に）
				float sc = 0.30f * (1.0f - t);
				sc = std::max(sc, 0.05f);
				particle.transform.scale.x = sc;
				particle.transform.scale.y = sc;

				// 軽く上昇を弱めて、中心に吸い込まれる印象を維持（任意）
				particle.velocity.y *= 0.985f;
			} else if (name == "charge_pulse") {
				// リングが広がる + 脈動（sin）
				float pulse = 1.0f + 0.08f * std::sin(t * 12.0f); // 12は脈動回数
				float sc = (0.55f + 1.35f * t) * pulse;

				particle.transform.scale.x = sc;
				particle.transform.scale.y = sc;

				// ちょい回転
				particle.transform.rotate.z += 0.10f;

				// 色は寿命の後半で少し薄く（alphaとは別で、白→青寄りに）
				// ※不要なら消してOK
				particle.color.x = 0.75f;
				particle.color.y = 0.90f;
				particle.color.z = 1.00f;
			}

			// 速度による移動
			particle.transform.translate.x += particle.velocity.x;
			particle.transform.translate.y += particle.velocity.y;
			particle.transform.translate.z += particle.velocity.z;

			// GPUバッファの最大数 (kInstanceNum) を超えないようにする
			if (numInstance < group.kInstanceNum) {

				Matrix4x4 worldMatrix =
					MyMath::MakeAffineMatrix(
						particle.transform.scale,
						particle.transform.rotate,
						particle.transform.translate
					);

				group.instancingData[numInstance].World = worldMatrix;
				group.instancingData[numInstance].WVP =
					MyMath::Multiply(MyMath::Multiply(worldMatrix, viewMatrix), projectionMatrix);

				group.instancingData[numInstance].color = Vector4(particle.color);
				++numInstance;
			}

			++it;
		}

		group.instanceCount = static_cast<uint32_t>(numInstance);
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
	// System::GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	for (auto& [name, group] : particleGroups) {

		// 頂点なし or インスタンスなしならスキップ
		if (group.vertices.empty() || group.instanceCount == 0) {
			continue;
		}

		System::GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &group.vertexBufferView);

		System::GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(0, System::GetSrvManager()->GetGPUDescriptorHandle(group.instancingSrvIndex));
		System::GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(1, System::GetSrvManager()->GetGPUDescriptorHandle(group.srvIndex));

		System::GetDxCommon()->GetCommandList()->DrawInstanced(static_cast<UINT>(group.vertices.size()), group.instanceCount, 0, 0);
	}
}

void ParticleManager::Emit(const std::string name, const Vector3& position, uint32_t count) {

	// 指定したパーティクルグループが存在するか確認
	auto it = particleGroups.find(name);
	if (it == particleGroups.end()) {
		assert(false && "Particle group not found.");
		return;
	}

	ParticleGroup& group = it->second;

	if (name == "trail") {
		// ★ 弾道用：ランダムは使わず、位置固定・短命の線を量産
		for (int i = 0; i < count; ++i) {
			group.particles.push_back(MakeTrailParticle(position));
		}
		return;
	}

	// ランダムエンジンの初期化
	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	// 指定した数だけパーティクルを発生
	for (uint32_t i = 0; i < count; ++i) {
		if (name == "explosion") {
			group.particles.push_back(MakeRandomParticle(randomEngine, position));
		} else if (name == "hit") {
			group.particles.push_back(MakeNewParticle(randomEngine, position));
		} else if (name == "muzzle") {
			group.particles.push_back(MakeMuzzleFlashParticle(randomEngine, position));
		} else if (name == "dust") {
			group.particles.push_back(MakeDustParticle(randomEngine, position));
		} else if (name == "ring") {
			group.particles.push_back(MakeRingParticle(randomEngine, position));
		} else if (name == "cylinder") {
			group.particles.push_back(MakeCylinderParticle(randomEngine, position));
		} else if (name == "moonLight") {
			group.particles.push_back(MakeRingParticle(randomEngine, position));
			group.particles.push_back(MakeMoonLightParticle(position, true));
			group.particles.push_back(MakeMoonLightParticle(position, false));
		} else if (name == "charge_core") {
			group.particles.push_back(MakeChargeCoreParticle(randomEngine, position));
		} else if (name == "charge_pulse") {
			group.particles.push_back(MakeChargePulseRingParticle(randomEngine, position));
		} else if (name == "ribbon") {
			spiralEmitter.position = position;
			spiralEmitter.count = 0;
			spiralEmitter.timer = 0.0f;
			spiralEmitter.active = true;
		}
	}
}

void ParticleManager::CreateParticleGeoup(const std::string name, const std::string textureFilePath, const std::string& particleType) {

	auto it = particleGroups.find(name);
	if (it != particleGroups.end()) {
		return;
	}

	ParticleGroup newParticle{};
	newParticle.materialData.textureFilePath = textureFilePath;

	MakeVertexData(newParticle, particleType);

	System::GetTextureManager()->LoadTexture(textureFilePath);
	uint32_t srvIndex = System::GetTextureManager()->GetTextureIndexByFilePath(textureFilePath);
	newParticle.srvIndex = srvIndex;

	newParticle.kInstanceNum = 0xffff;
	newParticle.instancingResource =
		System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(),
			sizeof(ParticleForGPU) * newParticle.kInstanceNum);
	newParticle.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&newParticle.instancingData));

	newParticle.instancingSrvIndex = System::GetSrvManager()->Allocate();
	System::GetSrvManager()->CreateSRVforStructuredBuffer(newParticle.instancingSrvIndex,
		newParticle.instancingResource.Get(),
		newParticle.kInstanceNum,
		sizeof(ParticleForGPU));

	particleGroups.emplace(name, std::move(newParticle));
}

// ランダムなパーティクル生成関数
Particle ParticleManager::MakeRandomParticle(std::mt19937& randomEngine, const Vector3& translate) {

	std::uniform_real_distribution<float> distribution(-0.5f, 0.5f);
	std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
	std::uniform_real_distribution<float> distTime(2.0f, 4.0f);

	Particle particle;
	Vector3 randomTranslate{ distribution(randomEngine), distribution(randomEngine), distribution(randomEngine) };
	particle.transform.scale = { 1.0f, 1.0f, 1.0f };
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
	particle.transform.translate = translate + randomTranslate;
	particle.velocity = { distribution(randomEngine), distribution(randomEngine), distribution(randomEngine) };
	particle.color = { distColor(randomEngine), distColor(randomEngine), distColor(randomEngine), 1.0f };
	particle.lifeTime = distTime(randomEngine);
	particle.currentTime = 0.0f;

	return particle;
}

Particle ParticleManager::MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate) {

	// --- 乱数設定（弱めに調整） ---
	std::uniform_real_distribution<float> distDir(-0.5f, 0.5f);     // 方向の散らばり小さく
	std::uniform_real_distribution<float> distSpeed(0.15f, 0.35f); // ★速度小さめ
	std::uniform_real_distribution<float> distLife(0.10f, 0.20f);  // ★かなり短命
	std::uniform_real_distribution<float> distScale(0.08f, 0.16f); // 粒を小さめに
	std::uniform_real_distribution<float> distColor(0.85f, 1.0f);  // 黄色～オレンジ

	Particle particle;

	// --- 方向（狭い範囲に調整） ---
	Vector3 dir{
		distDir(randomEngine),
		distDir(randomEngine) * 0.2f,  // 上下の散らばりもっと小さく
		distDir(randomEngine)
	};

	// 方向ゼロ防止
	if (dir.x == 0 && dir.y == 0 && dir.z == 0) {
		dir = { 0.0f, 0.0f, 1.0f };
	}
	dir = MyMath::Normalize(dir);

	float speed = distSpeed(randomEngine);

	// --- 初期位置 ---
	particle.transform.translate = translate;

	// --- 大きさ ---
	float sc = distScale(randomEngine);
	particle.transform.scale = { sc, sc, 1.0f };

	// --- 色（火花色） ---
	float g = distColor(randomEngine);
	particle.color = { 1.0f, g, 0.2f, 1.0f };

	// --- 速度（弱め） ---
	particle.velocity = {
		dir.x * speed,
		dir.y * speed,
		dir.z * speed
	};

	// --- 寿命（短命でシュッと消える） ---
	particle.lifeTime = distLife(randomEngine);
	particle.currentTime = 0.0f;

	return particle;
}

Particle ParticleManager::MakeDustParticle(std::mt19937 &randomEngine, const Vector3 &translate)
{
	std::uniform_real_distribution<float> distPos(-1.2f, 1.2f);   // 広めに散る
	std::uniform_real_distribution<float> distVelX(-0.05f, 0.05f);
	std::uniform_real_distribution<float> distVelZ(-0.05f, 0.05f);
	std::uniform_real_distribution<float> distVelY(0.02f, 0.08f); // 上方向へフワッと
	std::uniform_real_distribution<float> distLife(0.8f, 1.4f);

	Particle p;
	p.transform.scale = { 1.5f,1.5f,1.5f }; // 少し大きめ
	p.transform.rotate = { 0,0,0 };
	p.transform.translate = {
		translate.x + distPos(randomEngine),
		translate.y,
		translate.z + distPos(randomEngine)
	};

	p.velocity = {
		distVelX(randomEngine),
		distVelY(randomEngine),
		distVelZ(randomEngine)
	};

	// 砂っぽい薄い色
	p.color = { 0.6f, 0.55f, 0.45f, 1.0f };

	p.lifeTime = distLife(randomEngine);
	p.currentTime = 0.0f;

	return p;
}

Particle ParticleManager::MakeMuzzleFlashParticle(std::mt19937& randomEngine, const Vector3& translate) {

	std::uniform_real_distribution<float> distScale(0.4f, 0.7f); // 大きめのフラッシュ
	std::uniform_real_distribution<float> distRot(-3.14f, 3.14f);
	std::uniform_real_distribution<float> distLife(0.05f, 0.12f);
	std::uniform_real_distribution<float> distColor(0.8f, 1.0f);

	Particle p;

	// 発射位置
	p.transform.translate = translate;

	// 回転ランダム（ビルボードなので Z 回転だけで十分）
	p.transform.rotate = { 0.0f, 0.0f, distRot(randomEngine) };

	float sc = distScale(randomEngine);
	p.transform.scale = { sc, sc, 1.0f };

	// 明るい黄色〜白
	float g = distColor(randomEngine);
	p.color = { 1.0f, g, 0.2f, 1.0f };

	// マズルフラッシュは移動しない（その場に広がる）
	p.velocity = { 0.0f, 0.0f, 0.0f };

	p.lifeTime = distLife(randomEngine);
	p.currentTime = 0.0f;

	return p;
}

Particle ParticleManager::MakeTrailParticle(const Vector3& pos)
{
	Particle p;

	// 位置：弾の現在位置
	p.transform.translate = pos;

	// 細長くて明るい「線」っぽい粒
	float sx = 0.05f;   // 太さ
	float sy = 0.35f;   // 長さ
	p.transform.scale = { sx, sy, 1.0f };

	p.transform.rotate = { 0.0f, 0.0f, 0.0f };   // ビルボードなのでZ回転だけでもOK

	// 動かさない（その場に残像として残る）
	p.velocity = { 0.0f, 0.0f, 0.0f };

	// 色：白っぽい黄色
	p.color = { 1.0f, 0.95f, 0.6f, 1.0f };

	// 寿命：かなり短くしてスッと消える
	p.lifeTime = 0.06f;
	p.currentTime = 0.0f;

	return p;
}

Particle ParticleManager::MakeRingParticle(std::mt19937& randomEngine, const Vector3& translate) {

	std::uniform_real_distribution<float> distRotate(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);

	//
	Particle particle;
	particle.transform.scale = { 1.0f, 1.0f, 1.0f };
	particle.transform.rotate = { 0.0f, 0.0f, distRotate(randomEngine) };
	particle.transform.translate = translate;
	particle.velocity = { 0.0f, 0.0f, 0.0f };
	particle.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	particle.lifeTime = 1.0f;
	particle.currentTime = 0.0f;
	return particle;
}

Particle ParticleManager::MakeCylinderParticle(std::mt19937& randomEngine, const Vector3& translate) {

	std::uniform_real_distribution<float> distRotate(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);

	//
	Particle particle;
	particle.transform.scale = { 1.0f, 1.0f, 1.0f };
	particle.transform.rotate = { 0.0f, distRotate(randomEngine), 0.0f };
	particle.transform.translate = translate;
	particle.velocity = { 0.0f, 0.0f, 0.0f };
	particle.color = { 1.0f, 0.0f, 0.0f, 1.0f };
	particle.lifeTime = 0.1f;
	particle.currentTime = 0.0f;
	return particle;
}

Particle ParticleManager::MakeMoonLightParticle(const Vector3& translate, bool isVertical) {

	Particle particle;

	// 回転と位置
	particle.transform.rotate = isVertical ? Vector3{ 0.0f, 0.0f, 1.0f } : Vector3{ 0.0f, 0.0f, 2.0f };
	particle.transform.translate = translate;

	particle.velocity = { 0.0f, 0.0f, 0.0f };

	// 視認性の高い黄色
	particle.color = { 1.0f, 1.0f, 0.0f, 1.0f };

	particle.lifeTime = 1.0f;
	particle.currentTime = 0.0f;

	float rotSpeed = std::sin(particle.currentTime * 2.0f) * 0.05f;
	particle.transform.rotate.z += rotSpeed;
	float scaleBase = 1.0f;
	float scaleOffset = 0.2f * std::sin(particle.currentTime * 4.0f);
	particle.transform.scale = Vector3{ scaleBase + scaleOffset, scaleBase + scaleOffset, 1.0f };

	return particle;
}

Particle ParticleManager::MakeSpiralParticle(const Vector3& translate, float angleOffset) {

	Particle particle;

	// 初期位置：円周上に配置
	float radius = 2.0f;
	float angle = angleOffset;

	particle.transform.translate = { std::cos(angle) * radius + translate.x, translate.y, std::sin(angle) * radius + translate.z };

	particle.transform.scale = { 0.2f, 0.2f, 0.2f };
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };

	// 回転しながら下降するような速度ベクトル
	float angularSpeed = 0.1f;    // 回転速度
	float downwardSpeed = -0.05f; // 下降速度（Y軸方向）

	// 速度：角度の増加に従って円を描く＋下降
	particle.velocity = {
		-std::sin(angle) * radius * angularSpeed, // x方向：円運動
		downwardSpeed,                            // y方向：下降
		std::cos(angle) * radius * angularSpeed   // z方向：円運動
	};

	particle.color = { 1.0f, 0.3f, 1.0f, 1.0f };
	particle.lifeTime = 2.0f;
	particle.currentTime = 0.0f;

	return particle;
}

void ParticleManager::MakeVertexData(ParticleGroup& group, const std::string& particleType) {

	std::vector<VertexData> vertices;

	if (particleType == "ring") {

		const uint32_t kRingDivide = 32;
		const float kOuterRadius = 2.0f;
		const float kInnerRadius = 1.0f;
		const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kRingDivide);

		for (uint32_t index = 0; index < kRingDivide; ++index) {
			float sin = std::sin(index * radianPerDivide);
			float cos = std::cos(index * radianPerDivide);
			float sinNext = std::sin((index + 1) * radianPerDivide);
			float cosNext = std::cos((index + 1) * radianPerDivide);
			float u = float(index) / float(kRingDivide);
			float uNext = float(index + 1) / float(kRingDivide);

			vertices.push_back({
				{-sin * kOuterRadius, cos * kOuterRadius, 0.0f, 1.0f},
				{u, 0.0f},
				{0.0f, 0.0f, 1.0f}
				});
			vertices.push_back({
				{-sinNext * kOuterRadius, cosNext * kOuterRadius, 0.0f, 1.0f},
				{uNext, 0.0f},
				{0.0f, 0.0f, 1.0f}
				});
			vertices.push_back({
				{-sin * kInnerRadius, cos * kInnerRadius, 0.0f, 1.0f},
				{u, 1.0f},
				{0.0f, 0.0f, 1.0f}
				});
			vertices.push_back({
				{-sinNext * kOuterRadius, cosNext * kOuterRadius, 0.0f, 1.0f},
				{uNext, 0.0f},
				{0.0f, 0.0f, 1.0f}
				});
			vertices.push_back({
				{-sinNext * kInnerRadius, cosNext * kInnerRadius, 0.0f, 1.0f},
				{uNext, 1.0f},
				{0.0f, 0.0f, 1.0f}
				});
			vertices.push_back({
				{-sin * kInnerRadius, cos * kInnerRadius, 0.0f, 1.0f},
				{u, 1.0f},
				{0.0f, 0.0f, 1.0f}
				});
		}
	} else if (particleType == "cylinder") {

		const uint32_t kLineCount = 32;
		const float radius = 2.0f;

		// ランダム生成器
		std::random_device seed;
		std::mt19937 randEngine(seed());
		std::uniform_real_distribution<float> heightDist(1.5f, 2.5f); // 高さの範囲

		for (uint32_t i = 0; i < kLineCount; ++i) {
			float angle = float(i) / float(kLineCount) * 2.0f * std::numbers::pi_v<float>;
			float nextAngle = float(i + 1) / float(kLineCount) * 2.0f * std::numbers::pi_v<float>;

			float x0 = std::cos(angle) * radius;
			float z0 = std::sin(angle) * radius;

			float x1 = std::cos(nextAngle) * radius;
			float z1 = std::sin(nextAngle) * radius;

			// ランダムな高さ
			float height0 = heightDist(randEngine);
			float height1 = heightDist(randEngine);

			// 頂点位置
			Vector4 p0 = { x0, 0.0f, z0, 1.0f };
			Vector4 p1 = { x1, 0.0f, z1, 1.0f };
			Vector4 p2 = { x0, height0, z0, 1.0f };
			Vector4 p3 = { x1, height1, z1, 1.0f };

			Vector2 uvBottom = { 0.0f, 1.0f };
			Vector2 uvTop = { 0.0f, 0.0f };
			Vector3 normal = { 0.0f, 1.0f, 0.0f };

			// 三角形1
			vertices.push_back({ p0, uvBottom, normal });
			vertices.push_back({ p1, uvBottom, normal });
			vertices.push_back({ p2, uvTop, normal });

			// 三角形2
			vertices.push_back({ p2, uvTop, normal });
			vertices.push_back({ p1, uvBottom, normal });
			vertices.push_back({ p3, uvTop, normal });
		}
	} else if (particleType == "moonLight") {

		const uint32_t kRingDivide = 32;
		const float kOuterRadius = 2.0f;
		const float kInnerRadius = 1.0f;
		const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kRingDivide);

		for (uint32_t index = 0; index < kRingDivide; ++index) {
			float sin = std::sin(index * radianPerDivide);
			float cos = std::cos(index * radianPerDivide);
			float sinNext = std::sin((index + 1) * radianPerDivide);
			float cosNext = std::cos((index + 1) * radianPerDivide);
			float u = float(index) / float(kRingDivide);
			float uNext = float(index + 1) / float(kRingDivide);

			vertices.push_back({
				{-sin * kOuterRadius, cos * kOuterRadius, 0.0f, 1.0f},
				{u, 0.0f},
				{0.0f, 0.0f, 1.0f}
				});
			vertices.push_back({
				{-sinNext * kOuterRadius, cosNext * kOuterRadius, 0.0f, 1.0f},
				{uNext, 0.0f},
				{0.0f, 0.0f, 1.0f}
				});
			vertices.push_back({
				{-sin * kInnerRadius, cos * kInnerRadius, 0.0f, 1.0f},
				{u, 1.0f},
				{0.0f, 0.0f, 1.0f}
				});
			vertices.push_back({
				{-sinNext * kOuterRadius, cosNext * kOuterRadius, 0.0f, 1.0f},
				{uNext, 0.0f},
				{0.0f, 0.0f, 1.0f}
				});
			vertices.push_back({
				{-sinNext * kInnerRadius, cosNext * kInnerRadius, 0.0f, 1.0f},
				{uNext, 1.0f},
				{0.0f, 0.0f, 1.0f}
				});
			vertices.push_back({
				{-sin * kInnerRadius, cos * kInnerRadius, 0.0f, 1.0f},
				{u, 1.0f},
				{0.0f, 0.0f, 1.0f}
				});
		}
	} else {
		vertices = {
			{{1.0f, 1.0f, 0.0f, 1.0f},   {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
			{{-1.0f, 1.0f, 0.0f, 1.0f},  {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
			{{1.0f, -1.0f, 0.0f, 1.0f},  {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
			{{1.0f, -1.0f, 0.0f, 1.0f},  {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
			{{-1.0f, 1.0f, 0.0f, 1.0f},  {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
			{{-1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}
		};
	}

	// 頂点リソース作成
	group.vertices = vertices;
	group.vertexResource = System::GetDxCommon()->CreateBufferResource(System::GetDxCommon()->GetDevice(), sizeof(VertexData) * group.vertices.size());

	void* mappedData = nullptr;
	group.vertexResource->Map(0, nullptr, &mappedData);
	std::memcpy(mappedData, group.vertices.data(), sizeof(VertexData) * group.vertices.size());

	group.vertexBufferView.BufferLocation = group.vertexResource->GetGPUVirtualAddress();
	group.vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * group.vertices.size());
	group.vertexBufferView.StrideInBytes = sizeof(VertexData);
}

void ParticleManager::UpdateSpiralEmitter() {

	if (!spiralEmitter.active)
		return;

	spiralEmitter.timer += 1.0f / 60.0f;
	if (spiralEmitter.timer >= 0.05f) {
		float angle = spiralEmitter.count * 0.3f;
		auto& group = particleGroups["ribbon"];
		std::random_device seedGenerator;
		std::mt19937 randomEngine(seedGenerator());

		group.particles.push_back(MakeSpiralParticle(spiralEmitter.position, angle));

		spiralEmitter.count++;
		spiralEmitter.timer = 0.0f;

		if (spiralEmitter.count >= 24) {
			spiralEmitter.active = false;
		}
	}
}

bool ParticleManager::Exists(const std::string& name) const {
	return particleGroups.find(name) != particleGroups.end();
}

Particle ParticleManager::MakeChargeCoreParticle(std::mt19937& randomEngine, const Vector3& center) {

	// 円周上から中心へ吸い込まれる粒
	std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> distRadius(1.2f, 2.2f);
	std::uniform_real_distribution<float> distSpeed(0.08f, 0.16f);
	std::uniform_real_distribution<float> distLife(0.28f, 0.45f);
	std::uniform_real_distribution<float> distUp(0.002f, 0.008f);

	const float a = distAngle(randomEngine);
	const float r = distRadius(randomEngine);

	// 中心の周りにばら撒く
	Vector3 offset{ std::cos(a) * r, 0.0f, std::sin(a) * r };

	Particle p;
	p.transform.translate = center + offset;

	// ビルボード想定なので Z回転だけで十分
	p.transform.rotate = { 0.0f, 0.0f, a };

	// 小さめの粒
	p.transform.scale = { 0.25f, 0.25f, 1.0f };

	// 速度：中心へ向かう（XZ）
	Vector3 dir = MyMath::Normalize(Vector3{ -offset.x, 0.0f, -offset.z });
	float spd = distSpeed(randomEngine);

	p.velocity = { dir.x * spd, distUp(randomEngine), dir.z * spd };

	// 青白いチャージ色
	p.color = chargeCoreColor_;

	p.lifeTime = distLife(randomEngine);
	p.currentTime = 0.0f;

	return p;
}

Particle ParticleManager::MakeChargePulseRingParticle(std::mt19937& randomEngine, const Vector3& center) {

	std::uniform_real_distribution<float> distRotate(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> distLife(0.30f, 0.45f);

	Particle p;
	p.transform.translate = center;

	// リングは回転だけランダム
	p.transform.rotate = { 0.0f, 0.0f, distRotate(randomEngine) };

	// 初期サイズ（Updateで拡大させる想定）
	p.transform.scale = { 0.60f, 0.60f, 1.0f };

	// その場に留める
	p.velocity = { 0.0f, 0.0f, 0.0f };

	// 少し青寄りの白
	p.color = chargePulseColor_;

	p.lifeTime = distLife(randomEngine);
	p.currentTime = 0.0f;

	return p;
}