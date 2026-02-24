#include "TitleScene.h"

#include "Engine/Base/System/System.h"
#include "GameScene.h"

void TitleScene::Init() {

	System::GetOffscreenRendering()->SetPostEffect("none");

	// テクスチャ、モデルの読み込み
	TextureManager::GetInstance()->LoadTexture("./Resources/images/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/circle.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/circle2.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/test.dds");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/ground.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/Title.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/PushEnter.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/YOUDIE.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/hp.png");

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

	// Sprite
	titleSprite_ = std::make_unique<Sprite>();
	titleSprite_->Init("./Resources/images/Title.png", BlendType::BLEND_ADD);
	titleSprite_->SetSize({ 500.0f,400.0f });
	titleSprite_->SetPosition({ 460.0f,180.0f });

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
	fade_ = std::make_unique<Fade>();
	fade_->Initialize(1280, 720);
	if (Fade::GetDefaultOpenModeSlash()) {
		fade_->StartSlashOpen(0.6f, 60.0f, true);
	} else {
		fade_->Start(Fade::Status::FadeIn, 0.6f); // 普通の黒フェードで明転
	}

	// Player
	player_ = std::make_unique<Object3d>();
	player_->Init(BlendType::BLEND_NONE);
	player_->SetModel("titlePlayer.obj");
	player_->SetDefaultCamera(camera_.get());
	player_->SetRotate({ 0.0f, 0.65f, 0.0f });
	player_->SetTranslate({ -5.0f, -4.0f, 10.0f });

	// boss
	boss_ = std::make_unique<BossEnemy>();
	boss_->Init(camera_.get());
	boss_->SetTranslate({ 0.0f, 0.0f, 20.0f });
	boss_->SetAttack(false);
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
		fade_->Update();
		if (fade_->IsFinished()) {
			fade_->Stop();
			phase_ = Phase::kMain;
		}
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
			fade_->Start(Fade::Status::FadeOut, 0.6f);
			phase_ = Phase::kFadeOut;
		}
	}
	break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			sceneManager_->ChangeScene("TEST");   // ゲームへ
		}
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
	fade_->Draw();
}

void TitleScene::Finalize() {}