#include "GameScene.h"

#include "Engine/Base/System/System.h"
#include "Game/Scene/SceneManager.h"

#include <random>

#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif // DEBUG

void GameScene::Init() {

	// テクスチャのファイルパス
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

	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	ModelManager::GetInstance()->LoadModel("terrain.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");

	// 各種初期化
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({0.0f, 0.0f, 0.0f});
	camera_->SetTranslate({0.0f, 0.0f, -10.0f});

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
	emitter_->Init("hit", {0.0f, 0.0f, 10.0f}, 8);

	emitter2_ = std::make_unique<ParticleEmitter>();
	emitter2_->Init("explosion", {0.0f, 0.0f, 10.0f}, 50);

	ringEmitter_ = std::make_unique<ParticleEmitter>();
	ringEmitter_->Init("ring", {-4.0f, 0.0f, 10.0f}, 1);

	cylinderEmitter_ = std::make_unique<ParticleEmitter>();
	cylinderEmitter_->Init("cylinder", {4.0f, 0.0f, 10.0f}, 1);

	// Player
	player_ = std::make_unique<Player>();
	player_->Init(camera_.get());

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
		enemyObject->Init(BlendType::BLEND_NONE);
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

	// カメラのデフォルト位置を保存
	cameraDefaultPos_ = camera_->GetTranaslate();
}

void GameScene::Update() {

	camera_->Update();

	// Player
	player_->Update();

	// Enemy
	for (auto& enemy : enemies_) {
		enemy->Update();
	}

	// 当たり判定処理
	CheckCollisions();
	CameraShake();

	// シーン遷移のDebug処理
	if (System::GetInput()->TriggerKey(DIK_RETURN)) {
		sceneManager_->ChangeScene("TITLE");
	}

	ParticleUpdate();
	ImGuiDebug();
}

void GameScene::Draw() {

	// Player
	player_->Draw();

	// Enemy
	for (auto& enemy : enemies_) {
		enemy->Draw();
	}

	ParticleManager::GetInstance()->Draw();
}

void GameScene::Finalize() { ParticleManager::GetInstance()->Finalize(); }

void GameScene::CameraShake() {

	// カメラシェイク処理
	if (isShaking_) {
		shakeTimer_ += 1.0f / 60.0f;

		float shakeStrength = 0.2f;
		Vector3 shakeOffset = {(rand() % 100 / 100.0f - 0.5f) * 2.0f * shakeStrength, (rand() % 100 / 100.0f - 0.5f) * 2.0f * shakeStrength, 0.0f};

		camera_->SetTranslate(cameraDefaultPos_ + shakeOffset);

		if (shakeTimer_ >= shakeDuration_) {
			isShaking_ = false;
			camera_->SetTranslate(cameraDefaultPos_);
		}
	}
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
				// カメラシェイクのフラグを建てる
				isShaking_ = true;
				shakeTimer_ = 0.0f;

				// 衝突地点（弾と敵の中間地点）を計算
				Vector3 bulletPos = (*itBullet)->GetTranslate();
				Vector3 enemyPos = (*itEnemy)->GetTranslate();
				Vector3 hitPos = {(bulletPos.x + enemyPos.x) * 0.5f, (bulletPos.y + enemyPos.y) * 0.5f, (bulletPos.z + enemyPos.z) * 0.5f};

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

	// 敵が全ていなくなった場合
	if (enemies_.empty()) {
		sceneManager_->ChangeScene("CLEAR");
	}
}

void GameScene::ImGuiDebug() {

#ifdef _DEBUG

	camera_->ImGuiDebug();
	player_->ImGuiDebug();

#endif // _DEBUG
}

void GameScene::ParticleUpdate() {

	// パーティクルの更新処理
	ParticleManager::GetInstance()->Update();

#ifdef _DEBUG

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

	if (ImGui::Button("MoonLight Effect")) {
		moonLightEffect_->Update();
	}

	if (ImGui::Button("Ribbon Effect")) {
		ribbonEffect_->Update();
	}

#endif // _DEBUG

	if (System::TriggerKey(DIK_1)) {
		emitter_->Update();
	}
	if (System::TriggerKey(DIK_2)) {
		emitter2_->Update();
	}

	// ribbonEffect_->Update();

	ringEmitter_->Update();
	cylinderEmitter_->Update();
}