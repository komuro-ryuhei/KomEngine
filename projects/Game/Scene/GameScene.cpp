#include "GameScene.h"

#include "Engine/Base/System/System.h"
#include "Game/Scene/SceneManager.h"
#include "externals/nlohmann/json.hpp"

#include <random>

#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif // DEBUG

GameScene::GameScene() {}
GameScene::~GameScene() {}

void GameScene::Init() {

	camera_ = std::make_unique<Camera>();
	// camera_->SetRotate({0.2f, 0.0f, 0.0f});
	// camera_->SetTranslate({0.0f, 7.0f, -30.0f});
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 3.0f, -30.0f });

	// テクスチャの読み込み
	const std::string& uvTexture = "./Resources/images/uvChecker.png";
	const std::string& circle = "./Resources/images/circle.png";
	const std::string& circle2 = "./Resources/images/circle2.png";
	const std::string& monsterBallTexture = "./Resources/images/monsterBall.png";
	const std::string& ring = "./Resources/images/gradationLine.png";
	const std::string& moonLight = "./Resources/images/moonLight.png";

	// テクスチャ、モデルの読み込み
	TextureManager::GetInstance()->LoadTexture(uvTexture);
	TextureManager::GetInstance()->LoadTexture(circle);
	TextureManager::GetInstance()->LoadTexture(circle2);
	TextureManager::GetInstance()->LoadTexture(monsterBallTexture);
	TextureManager::GetInstance()->LoadTexture("./Resources/images/rostock_laage_airport_4k.dds");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/test.dds");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/ground.png");

	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	ModelManager::GetInstance()->LoadModel("terrain.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");
	ModelManager::GetInstance()->LoadModel("Player.obj");
	ModelManager::GetInstance()->LoadModel("Enemy.obj");
	ModelManager::GetInstance()->LoadModel("ground.obj");

	// Skybox
	skybox_ = std::make_unique<Skybox>();
	skybox_->Init("./Resources/images/test.dds");
	skybox_->SetDefaultCamera(camera_.get());

	// Sprite
	sprite_ = std::make_unique<Sprite>();
	sprite_->Init(uvTexture, BlendType::BLEND_NONE);

	object3d_ = std::make_unique<Object3d>();
	object3d_->Init(BlendType::BLEND_NONE);
	object3d_->SetModel("sphere.obj");
	object3d_->SetDefaultCamera(camera_.get());
	object3d_->SetEnvironmentTexture("./Resources/images/test.dds");

	glassObject_ = std::make_unique<Object3d>();
	glassObject_->Init(BlendType::BLEND_NONE);
	glassObject_->SetModel("ground.obj");
	glassObject_->SetDefaultCamera(camera_.get());

	audio_ = std::make_unique<Audio>();
	audio_->Init();

	SoundData soundData = audio_->SoundLoadWave("Resources/fanfare.wav");
	// audio_->SoundPlayWave(audio_->GetXAudio2(), soundData);

	// particle
	ParticleManager::GetInstance()->Init(camera_.get(), BlendType::BLEND_ADD);
	ParticleManager::GetInstance()->CreateParticleGeoup("hit", circle2, "a");
	ParticleManager::GetInstance()->CreateParticleGeoup("explosion", monsterBallTexture, "a");
	ParticleManager::GetInstance()->CreateParticleGeoup("ring", ring, "ring");
	ParticleManager::GetInstance()->CreateParticleGeoup("cylinder", ring, "cylinder");
	ParticleManager::GetInstance()->CreateParticleGeoup("moonLight", moonLight, "moonLight");
	ParticleManager::GetInstance()->CreateParticleGeoup("ribbon", moonLight, "ribbon");

	emitter_ = std::make_unique<ParticleEmitter>();
	emitter_->Init("hit", { 0.0f, 0.0f, 10.0f }, 8);

	emitter2_ = std::make_unique<ParticleEmitter>();
	emitter2_->Init("explosion", { 0.0f, 0.0f, 10.0f }, 50);

	ringEmitter_ = std::make_unique<ParticleEmitter>();
	ringEmitter_->Init("ring", { 0.0f, 0.0f, 10.0f }, 1);

	cylinderEmitter_ = std::make_unique<ParticleEmitter>();
	cylinderEmitter_->Init("cylinder", { 4.0f, 0.0f, 10.0f }, 1);

	// Player
	player_ = std::make_unique<Player>();
	player_->Init(camera_.get());

	// 敵の出現トリガー（例：Z=5.0f）
	enemyTriggers_.push_back({ {0.0f, 0.0f, 5.0f}, false });  // Z方向
	enemyTriggers_.push_back({ {10.0f, 0.0f, 5.0f}, false }); // X方向に回転したあとの位置

	moonLightEffect_ = std::make_unique<ParticleEmitter>();
	moonLightEffect_->Init("moonLight", { 0.0f, 0.0f, 10.0f }, 1);

	ribbonEffect_ = std::make_unique<ParticleEmitter>();
	ribbonEffect_->Init("ribbon", { 0.0f, 0.0f, 10.0f }, 1);

	loader_ = std::make_unique<Loader>();
	loader_->Init(camera_.get());
	ribbonEffect_->Init("ribbon", { 0.0f, 0.0f, 10.0f }, 1);

	// **ユーザー認証**
	// rankingManager.Login(L"komuro", L"password");
}

void GameScene::Update() {

	Vector3 playerPos = player_->GetTransform().translate;
	Vector3 playerRot = player_->GetTransform().rotate;

	camera_->SetBasePos(playerPos);
	camera_->SetBaseRot(playerRot);

	camera_->Update();

	object3d_->Update();
	glassObject_->Update();
	skybox_->Update();

	// Player
	player_->Update();

	if (isRotating_) {

		playerRot.y += rotateStep_;
		rotateFrameCount_++;

		if (rotateFrameCount_ >= rotateFrameMax_) {
			playerRot.y = targetRotationY_;
			isRotating_ = false;
		}

		player_->SetRotate(playerRot);
		camera_->SetBaseRot(playerRot);

		if (!isRotating_ && !hasSpawnedAfterRotate_) {
			SpawnEnemies();
			hasSpawnedAfterRotate_ = true;
		}
	}

	if (!isFighting_ && !isRotating_) {
		player_->RailMove();
	}

	// Enemy
	for (auto& enemy : enemies_) {
		enemy->Update();
	}

	// 当たり判定
	CheckCollisions();

	// トリガーチェック
	EnemySpawnTrigger();

	// パーティクル
	ParticleUpdate();

#ifdef _DEBUG

	// **ImGuiのデバッグ描画**
	ImGuiDebug();

	// Particle描画ImGui
	ParticleUpdate();

	// **ランキングの描画**
	// rankingManager.Render();

#endif // _DEBUG
}

void GameScene::Draw() {

	// 
	skybox_->Draw();

	// sprite_->Draw();

	// Debug用オブジェクトの描画
	// object3d_->Draw();
	// 地面
	glassObject_->Draw();

	// Player
	player_->Draw();

	// Enemy
	for (auto& enemy : enemies_) {
		enemy->Draw();
	}

	// loader_->Draw();

	ParticleManager::GetInstance()->Draw();
}

void GameScene::Finalize() { ParticleManager::GetInstance()->Finalize(); }

void GameScene::ImGuiDebug() {

#ifdef _DEBUG

	ChangePostEffect();

	camera_->ImGuiDebug();
	// object3d_->ImGuiDebug(); // オブジェクト
	// sprite_->ImGuiDebug(); // スプライト

	player_->ImGuiDebug();

	// **ランキングの描画**
	// rankingManager.Render();
	// シーン遷移のDebug処理
	if (System::GetInput()->TriggerKey(DIK_RETURN)) {
		sceneManager_->ChangeScene("TITLE");
	}

#endif // _DEBUG
}

void GameScene::ChangePostEffect() {

#ifdef _DEBUG
	// ポストエフェクトの選択肢
	static const char* effectItems[] = {
		"None", "Grayscale", "Vignetting", "Smoothing", "GaussinanFilter", "RadialBlur", "Random","Outline",
		"Glitch","Pixel", "ChromaticAberration", "VHSNoise","ColorInversion",
	};

	ImGui::Begin("PostEffect Settings");
	if (ImGui::Combo("Post Effect", &selectedPostEffectIndex_, effectItems, IM_ARRAYSIZE(effectItems))) {
		// エフェクト名を取得
		std::string selectedEffect = effectItems[selectedPostEffectIndex_];

	if (ImGui::Button("Reload Scene JSON")) {
		loader_->Reload(camera_.get());
	}

#endif // _DEBUG
}

void GameScene::ParticleUpdate() {

	// パーティクルの更新処理
	ParticleManager::GetInstance()->Update();

#ifdef _DEBUG

	ImGui::Begin("Particle Emitter");

	if (ImGui::Button("Emit Particles")) {
		emitter_->Update();
	}

	if (ImGui::Button("Emit2 Particles")) {
		emitter2_->Update();
	}

	if (ImGui::Button("Ring Particles")) {
		ringEmitter_->Update();
	}

	if (ImGui::Button("Cylinder Particles")) {
		cylinderEmitter_->Update();
	}

	ImGui::End();

#endif // _DEBUG

	if (System::TriggerKey(DIK_1)) {
		emitter_->Update();
	}
	if (System::TriggerKey(DIK_2)) {
		emitter2_->Update();
	}

	// ribbonEffect_->Update();

	/*ringEmitter_->Update();
	cylinderEmitter_->Update();*/
}

void GameScene::CheckCollisions() {

	auto& bullets = player_->GetBullets();

	// 敵と弾の当たり判定
	for (auto itBullet = bullets.begin(); itBullet != bullets.end();) {
		bool bulletHit = false;

		for (auto itEnemy = enemies_.begin(); itEnemy != enemies_.end();) {
			float distance = MyMath::CalculateDistance((*itBullet)->GetTranslate(), (*itEnemy)->GetTranslate());
			float collisionDistance = (*itBullet)->GetRadius() + (*itEnemy)->GetRadius();

			if (distance < collisionDistance) {
				// 衝突処理
				bulletHit = true;


				// 衝突地点（弾と敵の中間地点）を計算
				Vector3 bulletPos = (*itBullet)->GetTranslate();
				Vector3 enemyPos = (*itEnemy)->GetTranslate();
				Vector3 hitPos = { (bulletPos.x + enemyPos.x) * 0.5f, (bulletPos.y + enemyPos.y) * 0.5f, (bulletPos.z + enemyPos.z) * 0.5f };

				// 衝突位置からパーティクルを生成
				emitter_->SetTranslate(hitPos);
				emitter_->Update();

				// 敵を削除
				itEnemy = enemies_.erase(itEnemy);
			} else {
				++itEnemy;
			}
		}

		if (bulletHit) {
			// 衝突した弾を削除
			itBullet = bullets.erase(itBullet);
		} else {
			++itBullet;
		}
	}

	// 敵の弾とプレイヤーの当たり判定
	for (auto& enemy : enemies_) {
		for (auto it = enemy->GetBullets().begin(); it != enemy->GetBullets().end();) {
			float distance = MyMath::CalculateDistance((*it)->GetTranslate(), player_->GetTransform().translate);
			float collisionDistance = (*it)->GetRadius() + player_->GetRadius();

			if (distance < collisionDistance) {
				// 衝突処理（今は削除だけ）
				it = enemy->GetBullets().erase(it);
				// シェイクの処理初期化
				camera_->StartShake(CameraShakeType::Medium);
			} else {
				++it;
			}
		}
	}

	// プレイヤー弾 vs 敵弾
	for (auto& enemy : enemies_) {
		auto& enemyBullets = enemy->GetBullets();

		for (auto pbIt = player_->GetBullets().begin(); pbIt != player_->GetBullets().end();) {
			bool isHit = false;

			for (auto ebIt = enemyBullets.begin(); ebIt != enemyBullets.end();) {
				float distance = MyMath::CalculateDistance((*pbIt)->GetTranslate(), (*ebIt)->GetTranslate());

				float collisionDistance = (*pbIt)->GetRadius() + (*ebIt)->GetRadius();

				if (distance < collisionDistance) {
					// 衝突：両方削除
					ebIt = enemyBullets.erase(ebIt);
					pbIt = player_->GetBullets().erase(pbIt);
					isHit = true;
					break;
				} else {
					++ebIt;
				}
			}

			if (!isHit) {
				++pbIt;
			} else {
				break;
			}
		}
	}
}

void GameScene::SpawnEnemies() {

	const int enemyCount = 5;

	std::random_device rd;
	std::mt19937 gen(rd());

	// --- 横方向（side）の範囲 ---
	float sideMin = -5.0f;
	float sideMax = 5.0f;

	if (currentTriggerIndex_ == 1) {
		// 回転後は横方向（Z）をもっと奥に
		sideMin = -20.0f;
		sideMax = 20.0f;
	}

	std::uniform_real_distribution<float> distX(sideMin, sideMax);
	std::uniform_real_distribution<float> distY(-2.0f, 2.0f); // 高さ
	std::uniform_real_distribution<float> distZ(20.0f, 30.0f); // 前方向の奥行き

	Vector3 playerPos = player_->GetTransform().translate;

	Vector3 forward;
	Vector3 side;

	if (currentTriggerIndex_ == 0) {
		// 回転前 → Zが前
		forward = { 0.0f, 0.0f, 1.0f };
		side = { 1.0f, 0.0f, 0.0f };
	} else if (currentTriggerIndex_ == 1) {
		// 回転後 → Xが前
		forward = { 1.0f, 0.0f, 0.0f };
		side = { 0.0f, 0.0f, -1.0f };
	}

	for (int i = 0; i < enemyCount; ++i) {
		auto enemyObject = std::make_unique<Object3d>();
		enemyObject->Init(BlendType::BLEND_NONE);
		enemyObject->SetModel("sphere.obj");
		enemyObject->SetDefaultCamera(camera_.get());

		auto enemy = std::make_unique<Enemy>();
		enemy->Init(camera_.get(), enemyObject.get());
		enemy->SetPlayer(player_.get());
		enemy->Update();

		Vector3 pos = playerPos
			+ forward * distZ(gen)
			+ side * distX(gen)
			+ Vector3{ 0.0f, distY(gen), 0.0f };

		enemy->SetTranslate(pos);

		enemies_.emplace_back(std::move(enemy));
		enemyObjects3d_.emplace_back(std::move(enemyObject));
	}
}

void GameScene::EnemySpawnTrigger() {

	Vector3 playerPos = player_->GetTransform().translate;

	if (!isFighting_ && currentTriggerIndex_ < enemyTriggers_.size()) {
		auto& trigger = enemyTriggers_[currentTriggerIndex_];

		Vector3 diff = playerPos - trigger.triggerPos;
		float distance = sqrt(diff.x * diff.x + diff.z * diff.z);

		if (!trigger.triggered && distance < 1.0f) {
			SpawnEnemies();
			trigger.triggered = true;
			isFighting_ = true;
		}
	}

	if (isFighting_ && enemies_.empty()) {
		isFighting_ = false;
		currentTriggerIndex_++;

		if (currentTriggerIndex_ == 1) {
			isRotating_ = true;
			hasSpawnedAfterRotate_ = false;

			startRotationY_ = player_->GetTransform().rotate.y;
			targetRotationY_ = startRotationY_ + MyMath::DegreeToRadian(90.0f); // 90度

			rotateFrameCount_ = 0;
			rotateFrameMax_ = 30;  // 30フレームで回転
			rotateStep_ = (targetRotationY_ - startRotationY_) / (float)rotateFrameMax_;
		}
	}
}