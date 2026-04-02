#include "GameOverScene.h"

void GameOverScene::Init() {

	// カメラ
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.6f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 25.0f, -30.0f });
	KomEngine::System::GetParticleManager()->SetCamera(camera_.get());

	// スプライト
	sprite_ = std::make_unique<Sprite>();
	sprite_->Init("./Resources/images/YOUDIE.png", BlendType::BLEND_NONE);
	sprite_->SetSize({ 500.0f,200.0f });
	sprite_->SetPosition({ 640.0f,128.0f });

	// 倒れているプレイヤーのモデル
	downPlayer_ = std::make_unique<Object3d>();
	downPlayer_->Init(BlendType::BLEND_NONE);
	downPlayer_->SetModel("downHuman.obj");
	downPlayer_->SetDefaultCamera(camera_.get());
	downPlayer_->SetScale({ 2.0f, 2.0f, 2.0f });
	downPlayer_->SetTranslate({ 0.0f, 0.0f, 0.0f });

	// --- フェード初期化（画面サイズは 1280x720）--- //
	fade_ = std::make_unique<Fade>();
	fade_->Initialize(1280, 720);

	// 遷移元が指定した“開き方”に従う（ゲームオーバーは通常、黒フェード）
	if (Fade::GetDefaultOpenModeSlash()) {
		fade_->StartSlashOpen(0.6f, 60.0f, true);
	} else {
		fade_->Start(Fade::Status::FadeIn, 0.6f);
	}
	phase_ = Phase::kFadeIn;
}

void GameOverScene::Update() {

	// カメラの更新
	camera_->Update();

	// スプライトの更新
	sprite_->Update();

	// 倒れているプレイヤーオブジェクトの更新
	downPlayerRotate_.y += 0.01f;
	downPlayer_->SetRotate(downPlayerRotate_);
	downPlayer_->Update();

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			fade_->Stop();
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		// Enterでタイトルへ戻る
		if (KomEngine::System::TriggerKey(DIK_RETURN) || (KomEngine::System::TriggerKey(DIK_SPACE))) {
			fade_->Start(Fade::Status::FadeOut, 0.6f);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			// 
			KomEngine::System::GetOffscreenRendering()->SetPostEffect("none");
			// タイトルは斬撃で開きたい等、好みに応じて既定を設定
			Fade::SetDefaultOpenModeSlash(false);
			sceneManager_->ChangeScene("TITLE");
		}
		break;
	}
}

void GameOverScene::Draw() {

	// スプライト描画前処理
	sprite_->Draw();

	// 
	downPlayer_->Draw();

	// フェード描画
	if (fade_) { fade_->Draw(); }
}