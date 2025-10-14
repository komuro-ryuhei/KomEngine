#include "TitleScene.h"

#include "Engine/Base/System/System.h"
#include "GameScene.h"

void TitleScene::Init() {

	// テクスチャ、モデルの読み込み
	TextureManager::GetInstance()->LoadTexture("./Resources/images/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/circle.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/circle2.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/test.dds");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/ground.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/Title.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/PushEnter.png");

	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");
	ModelManager::GetInstance()->LoadModel("cube.obj");
	ModelManager::GetInstance()->LoadModel("Player.obj");
	ModelManager::GetInstance()->LoadModel("Enemy.obj");
	ModelManager::GetInstance()->LoadModel("ground.obj");
	ModelManager::GetInstance()->LoadModel("hand.obj");
	ModelManager::GetInstance()->LoadModel("BossEnemy.obj");

	// Sprite
	titleSprite_ = std::make_unique<Sprite>();
	titleSprite_->Init("./Resources/images/Title.png", BlendType::BLEND_NONE);
	titleSprite_->SetSize({ 0.0f,200.0f });
	titleSprite_->SetPosition({ 640.0f,256.0f });

	enterSprite_ = std::make_unique<Sprite>();
	enterSprite_->Init("./Resources/images/PushEnter.png", BlendType::BLEND_NONE);
	enterSprite_->SetPosition({ 640.0f,500.0f });

	// camera
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 0.0f, -10.0f });

	// Skybox
	skybox_ = std::make_unique<Skybox>();
	skybox_->Init("./Resources/images/test.dds");
	skybox_->SetDefaultCamera(camera_.get());

	// --- フェード初期化（画面サイズは 1280x720）---
	fade_ = std::make_unique<Fade>();
	fade_->Initialize(1280, 720);
	fade_->Start(Fade::Status::FadeIn, 0.6f);  // 入りで明転

	// boss
	boss_ = std::make_unique<BossEnemy>();
	boss_->Init(camera_.get());
	boss_->SetTranslate({ 0.0f, 0.0f, 20.0f });
	boss_->SetAttack(false);
	boss_->SetInTitleScene(true);
	boss_->InitTitleScenePos();
}

void TitleScene::Update() {

	// Sprite描画前処理
	// sprite_->PreDraw();

	// camera
	camera_->Update();
	// sprite
	titleSprite_->Update();
	titleSprite_->ImGuiDebug();
	titleSprite_->SetSize(titleSpriteScale_);

	// 
	enterSprite_->Update();
	enterSprite_->ImGuiDebug();

	if (boss_->GetIsmoveRight()) {
		if (titleSpriteScale_.x <= 500.0f) {
			titleSpriteScale_.x += 9.0f;
		} else {
			isPushEnter_ = true;
		}
	}

	// Skyboxの更新
	skybox_->Update();

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
		if (System::TriggerKey(DIK_RETURN)) {
			fade_->Start(Fade::Status::FadeOut, 0.6f);
			phase_ = Phase::kFadeOut;
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

	// タイトル
	titleSprite_->Draw();

	if (isPushEnter_) {
		enterSprite_->Draw();
	}

	// Bossの描画
	boss_->Draw();

	fade_->Draw();
}

void TitleScene::Finalize() {}