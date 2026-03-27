#include "TitleScene.h"

#include "Engine/Base/System/System.h"
#include "GameScene.h"

void TitleScene::Init() {

	System::GetOffscreenRendering()->SetPostEffect("none");

	// テクスチャ、モデルの読み込み
	System::GetTextureManager()->LoadTexture("./Resources/images/uvChecker.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/circle.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/circle2.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/test.dds");
	System::GetTextureManager()->LoadTexture("./Resources/images/ground.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/Title.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/PushEnter.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/YOUDIE.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/hp.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/white_1x1.png");

	System::GetTextureManager()->LoadTexture("./Resources/images/BossEnemyCore.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/BossEnemyMeteor.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/BossEnemyMissile.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/BossArmor.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/PlayerBullet.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/blackBG.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/controlsGuide.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/gameClear.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/gauge.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/heart.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/inner.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/mouseLeftClickWithText.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/mouseLightClickWithText.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/outer.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/pause.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/reticle.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/returnGame.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/toPause.png");
	System::GetTextureManager()->LoadTexture("./Resources/images/toTitle.png");

	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");
	ModelManager::GetInstance()->LoadModel("cube.obj");
	ModelManager::GetInstance()->LoadModel("Player.obj");
	ModelManager::GetInstance()->LoadModel("Enemy.obj");
	ModelManager::GetInstance()->LoadModel("ground.obj");
	ModelManager::GetInstance()->LoadModel("hand.obj");
	ModelManager::GetInstance()->LoadModel("BossEnemy.obj");
	ModelManager::GetInstance()->LoadModel("titlePlayer.obj");
	ModelManager::GetInstance()->LoadModel("gun.obj");
	ModelManager::GetInstance()->LoadModel("BossArmor.obj");
	ModelManager::GetInstance()->LoadModel("BossEnemyCore.obj");
	ModelManager::GetInstance()->LoadModel("PlayerBullet.obj");
	ModelManager::GetInstance()->LoadModel("BossEnemyMissile.obj");
	ModelManager::GetInstance()->LoadModel("BossEnemyMeteor.obj");

	// Sprite
	titleSprite_ = std::make_unique<Sprite>();
	titleSprite_->Init("./Resources/images/Title.png", BlendType::BLEND_ADD);
	titleSprite_->SetSize({ 640.0f,540.0f });
	titleSprite_->SetPosition({ 400.0f,-30.0f });

	enterSprite_ = std::make_unique<Sprite>();
	enterSprite_->Init("./Resources/images/PushEnter.png", BlendType::BLEND_ADD);
	enterSprite_->SetPosition({ 640.0f,500.0f });
	enterSprite_->SetSize({ 400.0f,300.0f });
	enterSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	// camera
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 0.0f, -10.0f });

	// Skybox
	skybox_ = std::make_unique<Skybox>();
	skybox_->Init("./Resources/images/test.dds");
	skybox_->SetDefaultCamera(camera_.get());

	// --- フェード初期化 --- //

	// Player
	player_ = std::make_unique<Object3d>();
	player_->Init(BlendType::BLEND_NONE);
	player_->SetModel("titlePlayer.obj");
	player_->SetDefaultCamera(camera_.get());
	player_->SetRotate({ 0.0f, 3.5f, 0.0f });
	player_->SetTranslate({ -5.0f, -4.0f, 10.0f });

	// boss
	boss_ = std::make_unique<BossEnemy>();
	boss_->Init(camera_.get());
	boss_->SetTranslate({ 0.0f, 0.0f, 20.0f });
	boss_->SetAttack(true);
	boss_->SetRotate({ -0.3f,0.85f,0.0f });
	boss_->SetTranslate({ 7.0f,4.0f,20.0f });

	// マウスカーソルを中央に固定を解除
	System::GetInput()->SetMouseCenterLock(false);
}

void TitleScene::Update() {

	const float dt = System::GetDeltaTime();

	// camera
	camera_->Update();
	// sprite
	titleSprite_->Update();
	titleSprite_->ImGuiDebug();

	// 
	enterSprite_->Update();
	enterSprite_->ImGuiDebug();

	// Skyboxの更新
	skybox_->Update();

	// Player
	player_->Update();
	player_->ImGuiDebug("player");

	// boss
	boss_->Update();
	boss_->ImGuiDebug();

	//if (System::TriggerKey(DIK_RETURN)) {
	//	// ゲームシーンを生成
	//	sceneManager_->ChangeScene("TEST");
	//}

	// 
	switch (phase_) {
	case Phase::kFadeIn:
		phase_ = Phase::kMain;
		break;

	case Phase::kMain:
	{
		// ---- PushEnter を徐々に点滅 ---- //
		pushBlinkTime_ += dt;

		const float omega = 2.0f * 3.14159265f / pushBlinkPeriod_;
		const float s = 0.5f + 0.5f * std::sinf(pushBlinkTime_ * omega); // 0～1
		const float a = pushBlinkMinA_ + (pushBlinkMaxA_ - pushBlinkMinA_) * s;

		enterSprite_->SetColor({ 1.0f, 1.0f, 1.0f, a });

		if (System::TriggerKey(DIK_RETURN) || System::TriggerKey(DIK_SPACE)) {
			phase_ = Phase::kFadeOut;
		}
	}
	break;

	case Phase::kFadeOut:
		sceneManager_->ChangeScene("TEST");
		break;
	}
}

void TitleScene::Draw() {

	// Skyboxの描画
	skybox_->Draw();

	/*if (isPushEnter_) {
		enterSprite_->Draw();
	}*/

	enterSprite_->Draw();

	// Playerの描画
	player_->Draw();

	// Bossの描画
	boss_->Draw();

	// タイトル
	titleSprite_->Draw();

	// フェード
}

void TitleScene::Finalize() {}