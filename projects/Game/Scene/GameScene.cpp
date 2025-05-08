#include "GameScene.h"

#include "Engine/Base/System/System.h"
#include "externals/imgui/imgui.h"
#include "Game/Scene/SceneManager.h"

#include <random>

void GameScene::Init() {

	// テクスチャの読み込み
	const std::string& uvTexture = "./Resources/images/uvChecker.png";
	const std::string& circle = "./Resources/images/circle.png";
	const std::string& circle2 = "./Resources/images/circle2.png";
	const std::string& monsterBallTexture = "./Resources/images/monsterBall.png";
	const std::string& ring = "./Resources/images/gradationLine.png";

	TextureManager::GetInstance()->LoadTexture(uvTexture);
	TextureManager::GetInstance()->LoadTexture(circle);
	TextureManager::GetInstance()->LoadTexture(circle2);
	TextureManager::GetInstance()->LoadTexture(monsterBallTexture);

	// Sprite
	sprite_ = std::make_unique<Sprite>();
	sprite_->Init(uvTexture);

	object3d_ = std::make_unique<Object3d>();
	object3d_->Init();

	glassObject_ = std::make_unique<Object3d>();
	glassObject_->Init();

	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	ModelManager::GetInstance()->LoadModel("terrain.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");

	object3d_->SetModel("sphere.obj");
	glassObject_->SetModel("terrain.obj");

	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({0.0f, 0.0f, 0.0f});
	camera_->SetTranslate({0.0f, 0.0f, -10.0f});
	// camera_->SetRotate({0.5f, 0.0f, 0.0f});
	// camera_->SetTranslate({0.0f, 7.0f, -12.0f});

	object3d_->SetDefaultCamera(camera_.get());
	glassObject_->SetDefaultCamera(camera_.get());

	audio_ = std::make_unique<Audio>();
	audio_->Init();

	SoundData soundData = audio_->SoundLoadWave("Resources/fanfare.wav");
	// audio_->SoundPlayWave(audio_->GetXAudio2(), soundData);

	// 
	ParticleManager::GetInstance()->Init(camera_.get());
	ParticleManager::GetInstance()->CreateParticleGeoup("hit", circle2, "a");
	ParticleManager::GetInstance()->CreateParticleGeoup("explosion", monsterBallTexture, "a");
	ParticleManager::GetInstance()->CreateParticleGeoup("ring", ring, "ring");
	ParticleManager::GetInstance()->CreateParticleGeoup("cylinder", ring, "cylinder");

	emitter_ = std::make_unique<ParticleEmitter>();
	emitter_->Init("hit", {0.0f, 0.0f, 10.0f}, 8);

	emitter2_ = std::make_unique<ParticleEmitter>();
	emitter2_->Init("explosion", {0.0f, 0.0f, 10.0f}, 50);

	ringEmitter_ = std::make_unique<ParticleEmitter>();
	ringEmitter_->Init("ring", {0.0f, 0.0f, 10.0f}, 1);

	cylinderEmitter_ = std::make_unique<ParticleEmitter>();
	cylinderEmitter_->Init("cylinder", {0.0f, 0.0f, 10.0f}, 1);

	// Player
	player_ = std::make_unique<Player>();
	player_->Init(camera_.get(), sprite_.get());

	// ランダムな値を生成するための設定
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> distX(-5.0f, 5.0f);  // X座標の範囲
	std::uniform_real_distribution<float> distY(-5.0f, 5.0f);  // Y座標の範囲
	std::uniform_real_distribution<float> distZ(20.0f, 50.0f); // Z座標の範囲
	// 敵の生成
	const int enemyCount = 5;
	for (int i = 0; i < enemyCount; ++i) {
		auto enemyObject = std::make_unique<Object3d>();
		enemyObject->Init();
		enemyObject->SetModel("sphere.obj");
		enemyObject->SetDefaultCamera(camera_.get());

		auto enemy = std::make_unique<Enemy>();
		enemy->Init(camera_.get(), enemyObject.get());

		// 敵の初期位置をランダムに設定
		float randomX = distX(gen);
		float randomY = distY(gen);
		float randomZ = distZ(gen);
		enemy->SetTranslate({randomX, randomY, randomZ});

		enemies_.emplace_back(std::move(enemy));
		enemyObjects3d_.emplace_back(std::move(enemyObject));
	}
}

void GameScene::Update() {

	// Sprite描画前処理
	// sprite_->PreDraw();

	camera_->Update();

	sprite_->Update();

	object3d_->Update();
	glassObject_->Update();

	sprite_->ImGuiDebug();

	//
	ParticleManager::GetInstance()->Update();

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
	// ringEmitter_->Update();
	// cylinderEmitter_->Update();

	// Player
	player_->Update();

	// Enemy
	for (auto& enemy : enemies_) {
		enemy->Update();
	}

	// 当たり判定処理
	CheckCollisions();

	// ロックオン処理
	if (System::TriggerKey(DIK_L)) {
		// Lキーでロックオン対象を更新
		player_->LockOnTarget(enemies_);
	}

	if (System::GetInput()->TriggerKey(DIK_RETURN)) {
		sceneManager_->ChangeScene("TITLE");
	}

	//
	camera_->ImGuiDebug();
	object3d_->ImGuiDebug();
	// glassObject_->ImGuiDebug();

	player_->ImGuiDebug();
}

void GameScene::Draw() {

	// sprite_->Draw();

	//
	// object3d_->Draw();

	// 地面
	// glassObject_->Draw();

	// Player
	player_->Draw();

	// Enemy
	for (auto& enemy : enemies_) {
		enemy->Draw();
	}

	ParticleManager::GetInstance()->Draw();
}

void GameScene::Finalize() { ParticleManager::GetInstance()->Finalize(); }

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

	// 敵が全ていなくなった場合
	if (enemies_.empty()) {
		sceneManager_->ChangeScene("CLEAR");
	}
}