#include "BossTestScene.h"
#include "externals/imgui/imgui.h"
#include "Engine/Base/System/System.h"

void BossTestScene::Init() {


	// テクスチャ、モデルの読み込み
	TextureManager::GetInstance()->LoadTexture("./Resources/images/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/circle.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/circle2.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/test.dds");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/ground.png");

	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");
	ModelManager::GetInstance()->LoadModel("cube.obj");
	ModelManager::GetInstance()->LoadModel("Player.obj");
	ModelManager::GetInstance()->LoadModel("Enemy.obj");
	ModelManager::GetInstance()->LoadModel("ground.obj");
	ModelManager::GetInstance()->LoadModel("hand.obj");
	ModelManager::GetInstance()->LoadModel("BossEnemy.obj");

	// 
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 0.0f, -30.0f });

	// Skybox
	skybox_ = std::make_unique<Skybox>();
	skybox_->Init("./Resources/images/test.dds");
	skybox_->SetDefaultCamera(camera_.get());

	// 地面
	glassObject_ = std::make_unique<Object3d>();
	glassObject_->Init(BlendType::BLEND_NONE);
	glassObject_->SetModel("ground.obj");
	glassObject_->SetDefaultCamera(camera_.get());

	// Player
	player_ = std::make_unique<Player>();
	player_->Init(camera_.get());

	// Boss
	boss_ = std::make_unique<BossEnemy>();
	boss_->Init(camera_.get());
	boss_->SetTranslate({ 0.0f, 0.0f, 20.0f });
	boss_->SetPlayer(player_.get());

	ParticleManager::GetInstance()->Init(camera_.get(), BlendType::BLEND_ADD);
	ParticleManager::GetInstance()->CreateParticleGeoup("hit", "./Resources/images/circle2.png", "hit");

	emitter_ = std::make_unique<ParticleEmitter>();
	emitter_->Init("hit", { 0.0f, 0.0f, 10.0f }, 10);
}

void BossTestScene::Update() {

	Vector3 playerPos = player_->GetTransform().translate;
	Vector3 playerRot = player_->GetTransform().rotate;

	// フラグがtrueだと追従
	if (isCameraFollowPlayer_) {
		camera_->SetTranslate(playerPos);
		camera_->SetRotate(playerRot);
	}

	// カメラの更新
	camera_->Update();

	// Skyboxの更新
	skybox_->Update();
	// 地面オブジェクトの更新
	glassObject_->Update();


	// ----------------------- ゲームオブジェクトの更新 ----------------------- //

	player_->Update();
	boss_->Update();

	CheckCollisions();

	// パーティクルの更新処理
	ParticleManager::GetInstance()->Update();

	// -------------------------------------------------------------------- //

#ifdef _DEBUG

	ImGui::Begin("BossTestScene");

	camera_->ImGuiDebug();
	player_->ImGuiDebug();
	boss_->ImGuiDebug();

	ImGui::Checkbox("isCameraFollowPlayer", &isCameraFollowPlayer_);

	ImGui::End();

#endif // _DEBUG

}

void BossTestScene::Draw() {

	// Skyboxの描画
	skybox_->Draw();
	// 地面オブジェクトの描画
	// glassObject_->Draw();

	// -------------------- ゲームオブジェクトシーンの描画 -------------------- //

	// Playerは一人称視点なので非描画
	player_->Draw();

	// Bossの描画
	boss_->Draw();

	ParticleManager::GetInstance()->Draw();

	// --------------------------------------------------------------------//
}

void BossTestScene::Finalize() {}

void BossTestScene::CheckCollisions() {

	// -------------------- 弾とボス部位の当たり判定 -------------------- //
	auto& bullets = player_->GetBullets();
	auto body = boss_->GetBody();
	auto left = boss_->GetLeftArm();
	auto right = boss_->GetRightArm();

	for (auto it = bullets.begin(); it != bullets.end();) {
		bool hit = false;

		// 各部位に対してチェック
		std::vector<Object3d*> parts = { body, left, right };
		for (auto part : parts) {
			float d = MyMath::CalculateDistance((*it)->GetTranslate(), part->GetTranslate());
			float r = (*it)->GetRadius() + part->GetRadius();

			if (d < r) {
				// パーティクルを出す位置を弾の位置に変更
				Vector3 hitPos = (*it)->GetTranslate();
				emitter_->SetTranslate(hitPos);
				emitter_->Update();

				boss_->AddHitToAttackingArm();

				it = bullets.erase(it);
				hit = true;
				break;
			}
		}

		if (!hit) {
			++it;
		}
	}

	// -------------------- 自機とボス部位の当たり判定 -------------------- //
	if (!player_ || !boss_) return;

	Vector3 playerPos = player_->GetTranslate();
	float playerRadius = player_->GetRadius();

	// 各腕との当たり判定
	struct ArmData {
		Object3d* object;
		std::string name;
	};

	std::vector<ArmData> arms = {
		{ boss_->GetLeftArm(),  "LeftArm" },
		{ boss_->GetRightArm(), "RightArm" }
	};

	for (const auto& arm : arms) {
		Vector3 armPos = arm.object->GetWorldPosition();
		float armRadius = arm.object->GetRadius();

		float distance = MyMath::CalculateDistance(playerPos, armPos);
		if (distance < (playerRadius + armRadius)) {
			// 無敵フラグがオフの時に引数分のダメージ
			if (!player_->GetInvincible()) {
				player_->Damage(1);
				player_->SetInvincible(true);
			}

			// HPが引数以下ならポストエフェクトを適応
			if (player_->IsLowHP(2)) {
				System::GetOffscreenRendering()->SetPostEffect("Vignetting");
			}

			// カメラを揺らす
			if (camera_) {
				camera_->StartShake(CameraShakeType::Medium);
			}
			break;
		}
	}
}