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

	BuildEmitTable();
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

				particle.transform.rotate.z += 0.10f;

				// 途中で少し膨らみ、最後に中心へ消える
				float sc = 0.10f + std::sin(t * 3.1415926f) * 0.10f;
				sc *= (1.0f - t * 0.55f);
				sc = std::max(sc, 0.025f);

				particle.transform.scale.x = sc;
				particle.transform.scale.y = sc;

				// 少しずつ中心へ引かれる感じ
				particle.velocity.x *= 1.012f;
				particle.velocity.y *= 1.012f;
				particle.velocity.z *= 1.012f;

				// 青白く、後半は白く抜ける
				Vector4 c;
				c.x = 0.72f + 0.25f * t;
				c.y = 0.90f + 0.08f * t;
				c.z = 1.00f;
				c.w = (1.0f - t) * 0.95f;

				particle.color = c;
			}

			else if (name == "charge_pulse") {

				// 小さめから広がる青白リング
				float baseScale = 0.65f + 2.4f * t; // 0.65 -> 3.05
				baseScale = std::max(baseScale, 0.25f);

				float pulse = 1.0f + 0.05f * std::sin(t * 10.0f);
				float sc = baseScale * pulse;

				particle.transform.scale.x = sc;
				particle.transform.scale.y = sc;

				particle.transform.rotate.z += 0.015f;

				Vector4 c;
				c.x = 0.80f + 0.15f * t;
				c.y = 0.92f + 0.05f * t;
				c.z = 1.00f;
				c.w = (1.0f - t) * 0.75f;

				particle.color = c;
			}
			
			else if (name == "missile_flame") {

				// 少し回す
				particle.transform.rotate.z += 0.06f;

				// 最初大きめ、後半しぼむ
				float k = 1.0f - t;
				float sc = 0.10f + k * 0.28f;
				particle.transform.scale.x = sc;
				particle.transform.scale.y = sc * (1.2f + 0.6f * k);

				// 少しずつ減速して煙っぽく
				particle.velocity.x *= 0.95f;
				particle.velocity.y *= 0.98f;
				particle.velocity.z *= 0.95f;

				// 白→黄→オレンジっぽく抜ける
				Vector4 c;
				c.x = 1.0f;
				c.y = 0.18f + 0.22f * k;
				c.z = 0.02f + 0.06f * k;
				c.w = (1.0f - t) * 0.85f;

				particle.color = c;
			}

			if (name == "charge_aura") {

				float sc = 1.10f + 0.14f * std::sin(t * 6.28318f);
				particle.transform.scale.x = sc;
				particle.transform.scale.y = sc;

				particle.transform.rotate.z += 0.003f;

				Vector4 c;
				c.x = 0.72f + 0.03f * t;
				c.y = 0.88f + 0.02f * t;
				c.z = 1.00f;
				c.w = (1.0f - t) * 0.07f;

				particle.color = c;
			}

			else if (name == "player_charge_line") {
				float len = 2.8f * (1.0f - t) + 0.8f;
				float wid = 0.14f * (1.0f - t) + 0.04f;

				particle.transform.scale.x = wid;
				particle.transform.scale.y = len;

				particle.velocity.x *= 1.020f;
				particle.velocity.y *= 1.020f;
				particle.velocity.z *= 1.020f;

				particle.color.x = 0.78f + 0.15f * t;
				particle.color.y = 0.90f + 0.06f * t;
				particle.color.z = 1.00f;
				particle.color.w = (1.0f - t) * 0.85f;
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
	KomEngine::System::GetDxCommon()->GetCommandList()->SetGraphicsRootSignature(pipelineManager_->GetRootSignature());

	// コマンド: PSO(Pipeline State Object)を設定
	KomEngine::System::GetDxCommon()->GetCommandList()->SetPipelineState(pipelineManager_->GetGraphicsPipelineState());

	// コマンド: プリミティブトポロジーを設定 (三角形リスト)
	KomEngine::System::GetDxCommon()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// コマンド: VBV(Vertex Buffer View)を設定
	// KomEngine::System::GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	for (auto& [name, group] : particleGroups) {

		// 頂点なし or インスタンスなしならスキップ
		if (group.vertices.empty() || group.instanceCount == 0) {
			continue;
		}

		KomEngine::System::GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &group.vertexBufferView);

		KomEngine::System::GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(0, KomEngine::System::GetSrvManager()->GetGPUDescriptorHandle(group.instancingSrvIndex));
		KomEngine::System::GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(1, KomEngine::System::GetSrvManager()->GetGPUDescriptorHandle(group.srvIndex));

		KomEngine::System::GetDxCommon()->GetCommandList()->DrawInstanced(static_cast<UINT>(group.vertices.size()), group.instanceCount, 0, 0);
	}
}

void ParticleManager::Emit(const std::string name, const Vector3& position, uint32_t count) {

	auto groupIt = particleGroups.find(name);
	if (groupIt == particleGroups.end()) {
		assert(false && "Particle group not found.");
		return;
	}

	auto emitIt = emitTable_.find(name);
	if (emitIt == emitTable_.end()) {
		assert(false && "Emit function not found.");
		return;
	}

	emitIt->second(groupIt->second, position, count);
}

void ParticleManager::CreateParticleGeoup(const std::string name, const std::string textureFilePath, const std::string& particleType) {

	auto it = particleGroups.find(name);
	if (it != particleGroups.end()) {
		return;
	}

	ParticleGroup newParticle{};
	newParticle.materialData.textureFilePath = textureFilePath;

	MakeVertexData(newParticle, particleType);

	KomEngine::System::GetTextureManager()->LoadTexture(textureFilePath);
	uint32_t srvIndex = KomEngine::System::GetTextureManager()->GetTextureIndexByFilePath(textureFilePath);
	newParticle.srvIndex = srvIndex;

	newParticle.kInstanceNum = 0xffff;
	newParticle.instancingResource =
		KomEngine::System::GetDxCommon()->CreateBufferResource(KomEngine::System::GetDxCommon()->GetDevice(),
			sizeof(ParticleForGPU) * newParticle.kInstanceNum);
	newParticle.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&newParticle.instancingData));

	newParticle.instancingSrvIndex = KomEngine::System::GetSrvManager()->Allocate();
	KomEngine::System::GetSrvManager()->CreateSRVforStructuredBuffer(newParticle.instancingSrvIndex,
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

	Vector4 color = { 1.0f, 0.0f, 0.0f , 1.0f };

	Particle particle;
	Vector3 randomTranslate{ distribution(randomEngine), distribution(randomEngine), distribution(randomEngine) };
	particle.transform.scale = { 0.3f, 0.3f, 0.3f };
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
	particle.transform.translate = translate + randomTranslate;
	particle.velocity = { distribution(randomEngine), distribution(randomEngine), distribution(randomEngine) };
	particle.color = { color };
	particle.lifeTime = distTime(randomEngine);
	particle.currentTime = 0.0f;

	return particle;
}

Particle ParticleManager::MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate) {

	// --- 乱数設定 --- //
	std::uniform_real_distribution<float> distDir(-0.5f, 0.5f);    // 方向の散らばり小さく
	std::uniform_real_distribution<float> distSpeed(0.15f, 0.35f); // 速度小さめ
	std::uniform_real_distribution<float> distLife(0.10f, 0.20f);  // かなり短命
	std::uniform_real_distribution<float> distScale(0.08f, 0.16f); // 粒を小さめに
	std::uniform_real_distribution<float> distColor(0.85f, 1.0f);  // 黄色～オレンジ

	Particle particle;

	// --- 方向 --- //
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

Particle ParticleManager::MakeDustParticle(std::mt19937& randomEngine, const Vector3& translate)
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
	group.vertexResource = KomEngine::System::GetDxCommon()->CreateBufferResource(KomEngine::System::GetDxCommon()->GetDevice(), sizeof(VertexData) * group.vertices.size());

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

	std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> distRadius(0.9f, 2.0f);
	std::uniform_real_distribution<float> distHeight(-0.45f, 0.45f);
	std::uniform_real_distribution<float> distSpeed(0.10f, 0.22f);
	std::uniform_real_distribution<float> distLife(0.22f, 0.40f);
	std::uniform_real_distribution<float> distScale(0.06f, 0.15f);

	const float a = distAngle(randomEngine);
	const float r = distRadius(randomEngine);
	const float h = distHeight(randomEngine);

	Vector3 offset{
		std::cos(a) * r,
		h,
		std::sin(a) * r
	};

	Particle p;
	p.transform.translate = center + offset;
	p.transform.rotate = { 0.0f, 0.0f, a };

	float sc = distScale(randomEngine);
	p.transform.scale = { sc, sc, 1.0f };

	// 中心へ向かう
	Vector3 dir = MyMath::Normalize(center - p.transform.translate);

	// 少しだけ回り込む成分を入れる
	Vector3 tangent{
		-dir.z,
		0.0f,
		dir.x
	};

	float spd = distSpeed(randomEngine);
	p.velocity = dir * spd + tangent * 0.035f;

	p.color = chargeCoreColor_;
	p.lifeTime = distLife(randomEngine);
	p.currentTime = 0.0f;

	return p;
}

Particle ParticleManager::MakeChargePulseRingParticle(std::mt19937& randomEngine, const Vector3& center) {

	std::uniform_real_distribution<float> distRotate(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> distLife(0.40f, 0.65f);

	Particle p;
	p.transform.translate = center;
	p.transform.rotate = { 0.0f, 0.0f, distRotate(randomEngine) };

	// 小さく始める
	p.transform.scale = { 0.65f, 0.65f, 1.0f };
	p.velocity = { 0.0f, 0.0f, 0.0f };

	p.color = chargePulseColor_;
	p.lifeTime = distLife(randomEngine);
	p.currentTime = 0.0f;

	return p;
}

Particle ParticleManager::MakeChargeAuraParticle(std::mt19937& randomEngine, const Vector3& center) {

	std::uniform_real_distribution<float> distLife(0.28f, 0.42f);
	std::uniform_real_distribution<float> distScale(1.8f, 2.6f);
	std::uniform_real_distribution<float> distJitter(-0.08f, 0.08f);
	std::uniform_real_distribution<float> distRot(-0.3f, 0.3f);

	Particle p;

	// ほぼ中心固定、少しだけ揺らす
	p.transform.translate = {
		center.x + distJitter(randomEngine),
		center.y + distJitter(randomEngine),
		center.z + distJitter(randomEngine)
	};

	float sc = distScale(randomEngine);
	p.transform.scale = { sc, sc, 1.0f };

	p.transform.rotate = { 0.0f, 0.0f, distRot(randomEngine) };

	// ほぼ動かさない
	p.velocity = { 0.0f, 0.0f, 0.0f };

	// 青白い発光
	p.color = { 0.78f, 0.92f, 1.00f, 0.72f };

	p.lifeTime = distLife(randomEngine);
	p.currentTime = 0.0f;

	return p;
}

Particle ParticleManager::MakePlayerChargeLineParticle(std::mt19937& randomEngine, const Vector3& center) {

	std::uniform_real_distribution<float> distX(-12.0f, 12.0f);
	std::uniform_real_distribution<float> distY(-7.0f, 7.0f);
	std::uniform_real_distribution<float> distZ(16.0f, 34.0f);
	std::uniform_real_distribution<float> distSpeed(0.70f, 1.35f);
	std::uniform_real_distribution<float> distLife(0.10f, 0.18f);

	Particle p;

	Vector3 startOffset{
		distX(randomEngine),
		distY(randomEngine),
		-distZ(randomEngine)
	};

	p.transform.translate = center + startOffset;

	Vector3 dir = MyMath::Normalize(center - p.transform.translate);
	float spd = distSpeed(randomEngine);
	p.velocity = dir * spd;

	// 進行方向を向かせる
	float angle = std::atan2(dir.y, dir.x);

	// 横長 streak.png を使うので +90度 回す
	p.transform.rotate = { 0.0f, 0.0f, angle + 1.5707963f };

	// scale は1回だけ
	p.transform.scale = { 0.18f, 2.8f, 1.0f };

	p.color = chargeCoreColor_;
	p.lifeTime = distLife(randomEngine);
	p.currentTime = 0.0f;

	return p;
}

Particle ParticleManager::MakeMissileFlameParticle(std::mt19937& randomEngine, const Vector3& pos, const Vector3& forward) {

	std::uniform_real_distribution<float> distBack(0.15f, 0.55f);
	std::uniform_real_distribution<float> distSide(-0.08f, 0.08f);
	std::uniform_real_distribution<float> distUp(0.01f, 0.08f);
	std::uniform_real_distribution<float> distScale(0.18f, 0.34f);
	std::uniform_real_distribution<float> distLife(0.10f, 0.22f);
	std::uniform_real_distribution<float> distG(0.08f, 0.25f);
	std::uniform_real_distribution<float> distA(0.75f, 1.0f);

	Particle p{};

	Vector3 dir = MyMath::Normalize(forward);

	// 後ろ方向
	Vector3 back = dir * -1.0f;

	// 横方向
	Vector3 side = { dir.z, 0.0f, -dir.x };
	if (MyMath::Length(side) < 0.0001f) {
		side = { 1.0f, 0.0f, 0.0f };
	}
	side = MyMath::Normalize(side);

	// 少しだけミサイル後方にずらして生成
	p.transform.translate =
		pos
		+ back * distBack(randomEngine)
		+ side * distSide(randomEngine)
		+ Vector3{ 0.0f, distUp(randomEngine), 0.0f };

	float sc = distScale(randomEngine);
	p.transform.scale = { sc, sc, 1.0f };
	p.transform.rotate = { 0.0f, 0.0f, 0.0f };

	// 後方へ流れつつ、少し上に立ちのぼる
	p.velocity =
		back * MyMath::Rand(0.10f, 0.22f) +
		side * distSide(randomEngine) * 0.25f +
		Vector3{ 0.0f, MyMath::Rand(0.01f, 0.04f), 0.0f };

	// 白黄〜オレンジ
	float g = distG(randomEngine);
	p.color = { 1.0f, distG(randomEngine), 0.03f, distA(randomEngine) };

	p.lifeTime = distLife(randomEngine);
	p.currentTime = 0.0f;

	return p;
}

void ParticleManager::BuildEmitTable() {

	emitTable_.clear();

	emitTable_["explosion"] = [this](ParticleGroup& group, const Vector3& position, uint32_t count) {
		EmitExplosion(group, position, count);
		};

	emitTable_["hit"] = [this](ParticleGroup& group, const Vector3& position, uint32_t count) {
		EmitHit(group, position, count);
		};

	emitTable_["muzzle"] = [this](ParticleGroup& group, const Vector3& position, uint32_t count) {
		EmitMuzzle(group, position, count);
		};

	emitTable_["dust"] = [this](ParticleGroup& group, const Vector3& position, uint32_t count) {
		EmitDust(group, position, count);
		};

	emitTable_["ring"] = [this](ParticleGroup& group, const Vector3& position, uint32_t count) {
		EmitRing(group, position, count);
		};

	emitTable_["cylinder"] = [this](ParticleGroup& group, const Vector3& position, uint32_t count) {
		EmitCylinder(group, position, count);
		};

	emitTable_["moonLight"] = [this](ParticleGroup& group, const Vector3& position, uint32_t count) {
		EmitMoonLight(group, position, count);
		};

	emitTable_["charge_core"] = [this](ParticleGroup& group, const Vector3& position, uint32_t count) {
		EmitChargeCore(group, position, count);
		};

	emitTable_["charge_pulse"] = [this](ParticleGroup& group, const Vector3& position, uint32_t count) {
		EmitChargePulse(group, position, count);
		};

	emitTable_["ribbon"] = [this](ParticleGroup& group, const Vector3& position, uint32_t count) {
		EmitRibbon(group, position, count);
		};

	emitTable_["trail"] = [this](ParticleGroup& group, const Vector3& position, uint32_t count) {
		EmitTrailGroup(group, position, count);
		};

	emitTable_["player_charge_line"] = [this](ParticleGroup& group, const Vector3& position, uint32_t count) {
		EmitPlayerChargeLine(group, position, count);
		};

	emitTable_["charge_aura"] = [this](ParticleGroup& group, const Vector3& position, uint32_t count) {
		EmitChargeAura(group, position, count);
		};
}

void ParticleManager::EmitExplosion(ParticleGroup& group, const Vector3& position, uint32_t count) {

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	for (uint32_t i = 0; i < count; ++i) {
		group.particles.push_back(MakeRandomParticle(randomEngine, position));
	}
}

void ParticleManager::EmitHit(ParticleGroup& group, const Vector3& position, uint32_t count) {

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	for (uint32_t i = 0; i < count; ++i) {
		group.particles.push_back(MakeNewParticle(randomEngine, position));
	}
}

void ParticleManager::EmitMuzzle(ParticleGroup& group, const Vector3& position, uint32_t count) {

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	for (uint32_t i = 0; i < count; ++i) {
		group.particles.push_back(MakeMuzzleFlashParticle(randomEngine, position));
	}
}

void ParticleManager::EmitDust(ParticleGroup& group, const Vector3& position, uint32_t count) {

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	for (uint32_t i = 0; i < count; ++i) {
		group.particles.push_back(MakeDustParticle(randomEngine, position));
	}
}

void ParticleManager::EmitRing(ParticleGroup& group, const Vector3& position, uint32_t count) {

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	for (uint32_t i = 0; i < count; ++i) {
		group.particles.push_back(MakeRingParticle(randomEngine, position));
	}
}

void ParticleManager::EmitCylinder(ParticleGroup& group, const Vector3& position, uint32_t count) {

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	for (uint32_t i = 0; i < count; ++i) {
		group.particles.push_back(MakeCylinderParticle(randomEngine, position));
	}
}

void ParticleManager::EmitMoonLight(ParticleGroup& group, const Vector3& position, uint32_t count) {

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	for (uint32_t i = 0; i < count; ++i) {
		group.particles.push_back(MakeRingParticle(randomEngine, position));
		group.particles.push_back(MakeMoonLightParticle(position, true));
		group.particles.push_back(MakeMoonLightParticle(position, false));
	}
}

void ParticleManager::EmitChargeCore(ParticleGroup& group, const Vector3& position, uint32_t count) {

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	for (uint32_t i = 0; i < count; ++i) {
		group.particles.push_back(MakeChargeCoreParticle(randomEngine, position));
	}
}

void ParticleManager::EmitChargePulse(ParticleGroup& group, const Vector3& position, uint32_t count) {

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	for (uint32_t i = 0; i < count; ++i) {
		group.particles.push_back(MakeChargePulseRingParticle(randomEngine, position));
	}
}

void ParticleManager::EmitRibbon(ParticleGroup& group, const Vector3& position, uint32_t count) {

	(void)group;
	(void)count;

	spiralEmitter.position = position;
	spiralEmitter.count = 0;
	spiralEmitter.timer = 0.0f;
	spiralEmitter.active = true;
}

void ParticleManager::EmitTrailGroup(ParticleGroup& group, const Vector3& position, uint32_t count) {

	for (uint32_t i = 0; i < count; ++i) {
		group.particles.push_back(MakeTrailParticle(position));
	}
}

void ParticleManager::EmitPlayerChargeLine(ParticleGroup& group, const Vector3& position, uint32_t count) {

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	for (uint32_t i = 0; i < count; ++i) {
		group.particles.push_back(MakePlayerChargeLineParticle(randomEngine, position));
	}
}

void ParticleManager::EmitChargeAura(ParticleGroup& group, const Vector3& position, uint32_t count) {

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	for (uint32_t i = 0; i < count; ++i) {
		group.particles.push_back(MakeChargeAuraParticle(randomEngine, position));
	}
}

void ParticleManager::EmitMissileFlame(const Vector3& pos, const Vector3& forward, uint32_t count) {

	auto it = particleGroups.find("missile_flame");
	if (it == particleGroups.end()) {
		return;
	}

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	for (uint32_t i = 0; i < count; ++i) {
		it->second.particles.push_back(MakeMissileFlameParticle(randomEngine, pos, forward));
	}
}