#include "GameScene.h"

#include "Engine/Base/System/System.h"
#include "Game/Scene/SceneManager.h"
#include "externals/nlohmann/json.hpp"

#include <random>

#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#include "BossTestScene.h"
#endif // DEBUG

GameScene::GameScene() {}
GameScene::~GameScene() {}

void GameScene::Init() {

	camera_ = std::make_unique<Camera>();
	// camera_->SetRotate({0.2f, 0.0f, 0.0f});
	// camera_->SetTranslate({0.0f, 7.0f, -30.0f});
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 0.0f, -30.0f });

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
	ModelManager::GetInstance()->LoadModel("cube.obj");
	ModelManager::GetInstance()->LoadModel("Player.obj");
	ModelManager::GetInstance()->LoadModel("Enemy.obj");
	ModelManager::GetInstance()->LoadModel("ground.obj");
	ModelManager::GetInstance()->LoadModel("hand.obj");
	ModelManager::GetInstance()->LoadModel("BossEnemy.obj");

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

	// 敵の出現トリガー
	enemyTriggers_.push_back({ {0.0f, 0.0f, 5.0f}, false }); // Z方向
	enemyTriggers_.push_back({ {0.0f, 0.0f, 10.0f}, false });

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

	// 背景やカメラなどの更新
	camera_->Update();
	object3d_->Update();
	glassObject_->Update();
	skybox_->Update();

	Vector3 playerPos = player_->GetTransform().translate;
	Vector3 playerRot = player_->GetTransform().rotate;

	// 位置だけ追従
	if (isCameraFollowPlayer_) {
		camera_->SetTranslate(playerPos + camFollowOffset_);
		if (camFollowRotation_) {
			camera_->SetRotate(playerRot);
		}
	}


	// Player
	player_->Update();

	if (!isFighting_) {
		player_->RailMove();
	}

	// Enemy
	for (auto& enemy : enemies_) {
		enemy->Update();
		enemy->ImGuiDebug();
	}

	UpdateJumpWave(NowSec());

	loader_->Update();

	// 当たり判定
	CheckCollisions();

	// トリガーチェック
	EnemySpawnTrigger();

	if (System::GetInput()->TriggerKey(DIK_RETURN)) {
		sceneManager_->ChangeScene("TEST");
	}

#ifdef _DEBUG

	// **ImGuiのデバッグ描画**
	ImGuiDebug();

	// Particle描画ImGui
	ParticleUpdate();

#ifdef _DEBUG

	ImGui::Begin("GameScene");

	ImGui::DragFloat3("playerpos", &playerPos.x, 0.01f);

	ImGui::End();

#endif // _DEBUG


#endif // _DEBUG
}

void GameScene::Draw() {

	// 背景(スカイボックス)の描画
	skybox_->Draw();

	// sprite_->Draw();

	// Debug用オブジェクトの描画
	// object3d_->Draw();
	// 地面
	// glassObject_->Draw();

	// プレイヤーの描画
	player_->Draw();

	// エネミーの描画
	for (auto& enemy : enemies_) {
		enemy->Draw();
	}

	// ステージエディターの描画
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

	ImGui::Checkbox("cameraFollow", &isCameraFollowPlayer_);

	// **ランキングの描画**
	// rankingManager.Render();
	// シーン遷移のDebug処理
	if (System::GetInput()->TriggerKey(DIK_RETURN)) {
		sceneManager_->ChangeScene("TEST");
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

		if (selectedEffect == "None") {
			System::GetOffscreenRendering()->SetPostEffect("none");
		} else {
			System::GetOffscreenRendering()->SetPostEffect(selectedEffect);
		}

		if (ImGui::Button("Reload Scene JSON")) {
			loader_->Reload(camera_.get());
		}
	}

	ImGui::End();

#endif // _DEBUG
}

// ===== 時間取得ヘルパ =====
double GameScene::NowSec() {
	using clock = std::chrono::steady_clock;
	auto now = clock::now().time_since_epoch();
	return std::chrono::duration<double>(now).count();
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
		enemyObject->SetModel("Enemy.obj");
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
			if (currentTriggerIndex_ == 0) {
				// 第1フェーズ: 通常スポーン
				SpawnEnemies();
			} else if (currentTriggerIndex_ == 1) {
				// 第2フェーズ: ジャンプ波（10体、0.5〜1.0秒ランダム）
				StartJumpWave(10, 0.5f, 1.0f);
			}
			trigger.triggered = true;
			isFighting_ = true;
		}
	}

	// 全滅したら戦闘終了 → 次のトリガーへ
	if (isFighting_ && !jumpWave_.active && enemies_.empty()) {
		isFighting_ = false;
		currentTriggerIndex_++;
	}
}

// ====== ジャンプ波：開始 ======
void GameScene::StartJumpWave(int count, float minIntervalSec, float maxIntervalSec) {

	jumpWave_.active = true;
	jumpWave_.toSpawn = count;
	jumpWave_.spawned = 0;
	jumpWave_.minInterval = minIntervalSec;
	jumpWave_.maxInterval = maxIntervalSec;
	jumpWave_.nextSpawnAt = NowSec();
}

// ====== ジャンプ波：毎フレーム更新 ======
void GameScene::UpdateJumpWave(double nowSec) {

	if (!jumpWave_.active) return;
	if (jumpWave_.spawned >= jumpWave_.toSpawn) {
		jumpWave_.active = false;
		return;
	}
	if (nowSec < jumpWave_.nextSpawnAt) return;

	// スポーン実行
	SpawnOneJumpingEnemy();
	jumpWave_.spawned++;

	// 次回までのランダム間隔（0.5〜1.0秒）
	std::uniform_real_distribution<float> dist(jumpWave_.minInterval, jumpWave_.maxInterval);
	jumpWave_.nextSpawnAt = nowSec + dist(rng_);
}

void GameScene::SpawnOneJumpingEnemy() {

	// Object3D
	auto enemyObj = std::make_unique<Object3d>();
	enemyObj->Init(BlendType::BLEND_NONE);
	enemyObj->SetModel("Enemy.obj");
	enemyObj->SetDefaultCamera(camera_.get());

	// Enemy
	auto enemy = std::make_unique<Enemy>();
	enemy->Init(camera_.get(), enemyObj.get());
	enemy->SetPlayer(player_.get());

	const Vector3 playerPos = player_->GetTransform().translate;

	// 着地点：zは前方固定(+12)、xだけランダム、y=0
	const float kLandZOffset = 15.0f;   // 着地の前方距離
	const float kSpawnSideX = 20.0f;   // 画面外(左右)の距離
	std::uniform_real_distribution<float> xrand(-3.0f, 3.0f);
	std::bernoulli_distribution          pickRight(0.5);

	float targetX = playerPos.x + xrand(rng_);                // Xだけ散らす
	float targetZ = playerPos.z + kLandZOffset;

	// 左右どちらの画面外から来るか
	bool fromRight = pickRight(rng_);
	float spawnX = playerPos.x + (fromRight ? +kSpawnSideX : -kSpawnSideX);
	float spawnZ = targetZ;                                 // 横から来るのでZは同じ帯

	Vector3 targetPos = { targetX, 0.0f, targetZ };          // 必ずここで静止
	Vector3 startPos = { spawnX,  0.0f, spawnZ };          // ここからジャンプ

	// 放物線ジャンプ：必ず target に着地するように Enemy 側で制御
	std::uniform_real_distribution<float> vy0(0.24f, 0.32f); // 見た目の弧用（未使用でもOK）
	enemy->StartJump(startPos, targetPos, 0.25f, vy0(rng_));

	enemyObj->SetTranslate(startPos);  // 生成位置をObject3dへ反映
	enemyObj->Update();                // 行列更新

	enemies_.emplace_back(std::move(enemy));
	enemyObjects3d_.emplace_back(std::move(enemyObj));
}