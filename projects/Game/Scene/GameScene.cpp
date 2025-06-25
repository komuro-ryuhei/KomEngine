#include "GameScene.h"

#include "Engine/Base/System/System.h"
#include "Game/Scene/SceneManager.h"
#include "externals/nlohmann/json.hpp"

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

	// 敵の出現トリガー（例：Z=5.0f）
	enemyTriggers_.push_back({5.0f, false});

	loader_ = std::make_unique<Loader>();
	loader_->Init(camera_.get());
}

void GameScene::Update() {

	Vector3 playerPos = player_->GetTransform().translate;
	Vector3 playerRot = player_->GetTransform().rotate;

	camera_->SetBasePos(player_->GetTransform().translate);
	camera_->SetBaseRot(player_->GetTransform().rotate);

	camera_->Update();

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

	// 当たり判定処理
	CheckCollisions();
	// カメラシェイク処理
	// CameraShake();

	loader_->Update();

	//
	EnemySpawnTrigger();

	// パーティクル全体の更新
	ParticleUpdate();

#ifdef _DEBUG

	// シーン遷移のDebug処理
	if (System::GetInput()->TriggerKey(DIK_RETURN)) {
		sceneManager_->ChangeScene("TITLE");
	}

	ImGuiDebug();

#endif // _DEBUG
}

void GameScene::Draw() {

	// Player
	player_->Draw();

	// Enemy
	for (auto& enemy : enemies_) {
		enemy->Draw();
	}

	loader_->Draw();

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

void GameScene::SpawnEnemies() {

	// 敵の数
	const int enemyCount = 5;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> distX(-5.0f, 5.0f);
	std::uniform_real_distribution<float> distY(-2.0f, 2.0f);
	std::uniform_real_distribution<float> distZ(25.0f, 50.0f);

	// プレイヤーの現在位置
	Vector3 playerPos = player_->GetTransform().translate;

	// 敵の生成
	for (int i = 0; i < enemyCount; ++i) {
		auto enemyObject = std::make_unique<Object3d>();
		enemyObject->Init(BlendType::BLEND_NONE);
		enemyObject->SetModel("sphere.obj");
		enemyObject->SetDefaultCamera(camera_.get());

		auto enemy = std::make_unique<Enemy>();
		enemy->Init(camera_.get(), enemyObject.get());
		enemy->SetPlayer(player_.get());
		enemy->Update();

		Vector3 pos = {distX(gen), distY(gen), playerPos.z + distZ(gen)};
		enemy->SetTranslate(pos);

		enemies_.emplace_back(std::move(enemy));
		enemyObjects3d_.emplace_back(std::move(enemyObject));
	}
}

void GameScene::EnemySpawnTrigger() {

	// プレイヤーの奥行きをチェック
	float playerZ = player_->GetTransform().translate.z;

	// 敵出現チェック
	if (!isFighting_ && currentTriggerIndex_ < enemyTriggers_.size()) {
		auto& trigger = enemyTriggers_[currentTriggerIndex_];

		if (!trigger.triggered && playerZ >= trigger.triggerZ) {
			SpawnEnemies(); // 敵を出現させる
			trigger.triggered = true;
			isFighting_ = true;
		}
	}

	// 戦闘中かつ敵がいなくなったら進行再開
	if (isFighting_ && enemies_.empty()) {
		isFighting_ = false;
		currentTriggerIndex_++;
	}
}