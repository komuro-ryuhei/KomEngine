#include "BossTestScene.h"
#include "externals/imgui/imgui.h"

void BossTestScene::Init() {

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


	// -------------------- ゲームオブジェクトシーンの更新 -------------------- //

	player_->Update();
	boss_->Update();

	CheckCollisions();

	// ------------------------------------------------------------------- //

#ifdef _DEBUG

	ImGui::Begin("BossTestScene");

	camera_->ImGuiDebug();
	boss_->ImGuiDebug();

	ImGui::Checkbox("isCameraFollowPlayer", &isCameraFollowPlayer_);

	ImGui::End();

#endif // _DEBUG

}

void BossTestScene::Draw() {

	// Skyboxの描画
	skybox_->Draw();
	// 地面オブジェクトの描画
	glassObject_->Draw();

	// -------------------- ゲームオブジェクトシーンの描画 -------------------- //

	// Playerは一人称視点なので非描画
	player_->Draw();

	// Bossの描画
	boss_->Draw();

	// --------------------------------------------------------------------//
}

void BossTestScene::Finalize() {}

void BossTestScene::CheckCollisions() {

	// ----- 弾とボス部位の当たり判定 -----
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
				// 当たり
				it = bullets.erase(it);
				hit = true;
				break;
			}
		}

		if (!hit) {
			++it;
		}
	}
}