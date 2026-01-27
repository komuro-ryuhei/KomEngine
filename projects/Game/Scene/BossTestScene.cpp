#include "BossTestScene.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include "Engine/Base/System/System.h"

#include <memory>
#include <vector>

void BossTestScene::Init() {

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
	TextureManager::GetInstance()->LoadTexture("./Resources/images/test.dds");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/ground.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/reticle.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/inner.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/outer.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/hp.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/blackBG.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/gameClear.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/controlsGuide.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/toTitle.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/returnGame.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/pause.png");

	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");
	ModelManager::GetInstance()->LoadModel("cube.obj");
	ModelManager::GetInstance()->LoadModel("Player.obj");
	ModelManager::GetInstance()->LoadModel("Enemy.obj");
	ModelManager::GetInstance()->LoadModel("ground.obj");
	ModelManager::GetInstance()->LoadModel("hand.obj");
	ModelManager::GetInstance()->LoadModel("BossEnemy.obj");
	ModelManager::GetInstance()->LoadModel("gun.obj");

	// カメラ
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	// camera_->SetTranslate({ 0.0f, 0.0f, -30.0f });

	// Skybox
	skybox_ = std::make_unique<Skybox>();
	skybox_->Init("./Resources/images/test.dds");
	skybox_->SetDefaultCamera(camera_.get());

	// デバッグライン
	debugLine_.Init(2048, BlendType::BLEND_ALPHA);
	debugLine_.SetCamera(camera_.get());

	// 地面
	glassObject_ = std::make_unique<Object3d>();
	glassObject_->Init(BlendType::BLEND_NONE);
	glassObject_->SetModel("ground.obj");
	glassObject_->SetDefaultCamera(camera_.get());
	glassObject_->SetTranslate({ 0.0f, -5.0f, 0.0f });

	// --- フェード初期化（画面サイズは 1280x720）---
	fade_ = std::make_unique<Fade>();
	fade_->Initialize(1280, 720);
	if (Fade::GetDefaultOpenModeSlash()) {
		fade_->StartSlashOpen(0.6f, 60.0f, true);
	} else {
		fade_->Start(Fade::Status::FadeIn, 0.6f);
	}
	phase_ = Phase::kFadeIn;

	// Player
	player_ = std::make_unique<Player>();
	player_->Init(camera_.get());

	// Playerが持つ銃
	gun_ = std::make_unique<Object3d>();
	gun_->Init(BlendType::BLEND_NONE);
	gun_->SetModel("gun.obj");
	gun_->SetDefaultCamera(camera_.get());

	// Boss
	boss_ = std::make_unique<BossEnemy>();
	boss_->Init(camera_.get());
	boss_->SetTranslate({ 0.0f, 0.0f, 20.0f });
	bossSpawnPos_ = boss_->GetTranslate();
	boss_->SetPlayer(player_.get());

	auto makeTarget = [](std::unique_ptr<Sprite>& outer, std::unique_ptr<Sprite>& inner)
		{
			outer = std::make_unique<Sprite>();
			outer->Init("./Resources/images/outer.png", BlendType::BLEND_ALPHA);
			outer->SetAnchorPoint({ 0.5f, 0.5f });
			outer->SetSize({ 128,128 });

			inner = std::make_unique<Sprite>();
			inner->Init("./Resources/images/inner.png", BlendType::BLEND_ALPHA);
			inner->SetAnchorPoint({ 0.5f, 0.5f });
			inner->SetSize({ 128,128 });
		};

	makeTarget(leftTargetOuter_, leftTargetInner_);
	makeTarget(rightTargetOuter_, rightTargetInner_);

	// ボスのメテオ
	meteors_.clear();
	meteors_.reserve(32);
	for (int i = 0; i < 32; ++i) {
		auto m = std::make_unique<BossMeteor>();
		m->Init(camera_.get());
		meteors_.push_back(std::move(m));
	}

	// メテオを CollisionManager に登録
	for (auto& m : meteors_) {
		collisionManager_.Register(m.get());
	}

	// ボスの攻撃管理
	attackManager_ = std::make_unique<BossAttackManager>();

	BossAttackManager::InitDesc init{};
	init.camera = camera_.get();
	init.player = player_.get();
	init.boss = boss_.get();
	init.meteors = &meteors_;

	attackManager_->Init(init);

	// jsonの読み込み
	attackManager_->GetMeteor()->LoadParamsFromJson("Resources/json/bossAttacks.json");
	attackManager_->GetArm()->LoadParamsFromJson("Resources/json/bossAttacks.json");

	// 
	controlGuideSprite_ = std::make_unique<Sprite>();
	controlGuideSprite_->Init("./Resources/images/controlsGuide.png", BlendType::BLEND_ALPHA);
	controlGuideSprite_->SetSize({ 400.0f, 280.0f });
	controlGuideSprite_->SetPosition({ 900.0f, 420.0f });

	// パーティクル
	auto* pm = ParticleManager::GetInstance();
	pm->Init(camera_.get(), BlendType::BLEND_ADD);

	pm->CreateParticleGeoup("hit", circle2, "a");
	pm->CreateParticleGeoup("explosion", monsterBallTexture, "a");
	pm->CreateParticleGeoup("ring", ring, "ring");
	pm->CreateParticleGeoup("cylinder", ring, "cylinder");
	pm->CreateParticleGeoup("moonLight", moonLight, "moonLight");
	pm->CreateParticleGeoup("ribbon", moonLight, "ribbon");
	pm->CreateParticleGeoup("dust", "./Resources/images/circle.png", "a");
	pm->CreateParticleGeoup("muzzle", circle2, "a");
	pm->CreateParticleGeoup("trail", "./Resources/images/circle.png", "a");
	pm->CreateParticleGeoup("charge_core", "./Resources/images/circle2.png", "a");
	pm->CreateParticleGeoup("charge_pulse", ring, "ring");

	// グループが既にあれば作らない
	if (!pm->Exists("hit")) {
		pm->CreateParticleGeoup("hit", "./Resources/images/circle2.png", "hit");
	}

	emitter_ = std::make_unique<ParticleEmitter>();
	emitter_->Init("hit", { 0.0f, 0.0f, 10.0f }, 10);

	result_ = std::make_unique<ResultImage>();
	result_->Init();

	// ---- CollisionManager 設定 ----
	collisionManager_.AddPairRule(CollisionLayer::Player, CollisionLayer::Enemy);
	collisionManager_.AddPairRule(CollisionLayer::Player, CollisionLayer::EnemyBullet);
	collisionManager_.AddPairRule(CollisionLayer::PlayerBullet, CollisionLayer::Enemy);
	collisionManager_.AddPairRule(CollisionLayer::PlayerBullet, CollisionLayer::EnemyBullet);

	// AddComponent 的な登録
	collisionManager_.Register(player_.get());
	collisionManager_.Register(boss_.get());

	player_->SetCollisionManager(&collisionManager_);
	boss_->SetCollisionManager(&collisionManager_);

	// Playで使う正規の位置を保存
	playCameraPos_ = camera_->GetTranaslate();
	playCameraRot_ = camera_->GetRotate();

	bossPlayPos_ = boss_->GetTranslate();

	// ポーズメニュー
	pauseMenu_ = std::make_unique<PauseMenu>();
	pauseMenu_->Init();
}

void BossTestScene::Update() {

	const float dt = System::GetDeltaTime();

	if (phase_ == Phase::kFadeIn) {
		fade_->Update();
		if (fade_->IsFinished()) {
			fade_->Stop();
			phase_ = Phase::kMain;
			// フェードが終わった瞬間に Intro を開始
			InitIntro();
		}
		return;
	}

	if (flowState_ == GameFlowState::Intro) {
		UpdateIntro(dt);

		// 見た目の更新だけはやっておく（最低限）
		camera_->Update();
		skybox_->Update();
		boss_->Update();   // combatEnabled_ が false なら攻撃しない
		ImGuiDebug();
		return;
	}

	// ---------------- Pause (Play中だけ) ---------------- //
	const bool canPause =
		(phase_ == Phase::kMain) &&
		(flowState_ == GameFlowState::Play) &&
		!(result_ && result_->IsSlideFinished());

	// Pキーでポーズメニューの表示
	if (canPause && System::TriggerKey(DIK_P) && pauseMenu_) {
		pauseMenu_->Toggle();
	}

	if (pauseMenu_->IsPaused()) {
		System::GetInput()->SetMouseCenterLock(false);
	}

	if (pauseMenu_ && pauseMenu_->IsPaused()) {

		const auto r = pauseMenu_->Update(dt);

		if (r == PauseMenu::Result::Resume) {
			pauseMenu_->SetPaused(false);
		} else if (r == PauseMenu::Result::GoTitle) {

			// ポーズ解除（重要：FadeOut更新へ到達させるため）
			pauseMenu_->SetPaused(false);
			System::GetInput()->SetMouseCenterLock(true); // 普段ロックしてるなら戻す（任意）

			// フェードアウト開始
			fade_->Start(Fade::Status::FadeOut, 0.6f);
			phase_ = Phase::kFadeOut;
			endReason_ = EndReason::GoTitle;

			return; // ここで抜けるのはOK（次フレームはpausedじゃないのでFadeOut進む）
		}

		ImGuiDebug(); // ポーズ中もデバッグは出す
		return;
	}
	// ---------------------------------------------------

	UpdateMeteorControl();

	// プレイヤー死亡処理
	UpdatePlayerDeath(dt);

	UpdateCamera(dt);

	// ノックアウトカメラの更新
	//if (koActive_) {
	//	ko_.Update(dt, camera_.get());
	//	if (ko_.IsDone()) {
	//		koActive_ = false;

	//		// フェードアウト開始
	//		if (fade_ && phase_ == Phase::kMain) {
	//			fade_->Start(Fade::Status::FadeOut, 0.6f);
	//			phase_ = Phase::kFadeOut;
	//			if (endReason_ == EndReason::None) {
	//				endReason_ = EndReason::PlayerDeath;
	//			}
	//		}
	//	}
	//}

	// デバッグ：離脱攻撃要求
	if (System::GetInput()->PushKey(DIK_R)) {
		boss_->StartRetreatAttack();
	}

	// デバッグ：チャージ攻撃要求
	if (System::GetInput()->PushKey(DIK_C)) {

		// 既にチャージ攻撃が動いているなら要求しない
		if (!boss_->IsChargeActive()) {

			bool targetLeft = !boss_->IsLeftArmBroken();
			if (boss_->IsLeftArmBroken() && !boss_->IsRightArmBroken()) {
				targetLeft = false;
			}
			boss_->RequestChargeAttack(targetLeft);
		}
	}

	if (System::GetInput()->PushKey(DIK_T)) {
		boss_->SetAttack(false);
	}

	// ----------------------- ゲームオブジェクトの更新 ----------------------- //

	BossAttackManager::UpdateFlags f{};
	f.koActive = koActive_;
	f.isMainPhase = (phase_ == Phase::kMain);
	f.isCameraFollowPlayer = isCameraFollowPlayer_;

	if (attackManager_) {
		attackManager_->Update(dt, f);
	}


	// ターゲットシェイク時間更新
	if (leftTargetShakeTime_ > 0.0f) {
		leftTargetShakeTime_ -= dt;
		if (leftTargetShakeTime_ < 0.0f) leftTargetShakeTime_ = 0.0f;
	}
	if (rightTargetShakeTime_ > 0.0f) {
		rightTargetShakeTime_ -= dt;
		if (rightTargetShakeTime_ < 0.0f) rightTargetShakeTime_ = 0.0f;
	}

	// カメラの更新
	camera_->Update();
	// Skyboxの更新
	skybox_->Update();
	// 地面オブジェクトの更新
	glassObject_->Update();

	// ライン側でカメラ行列更新
	debugLine_.Update();

	// ここから AddLine だけ書けばいい
	LineTarget();

	// Playerの銃の更新
	UpdateGun();
	// Playerの更新()
	player_->Update();

	// ボス
	boss_->Update();
	// ボスのメテオ攻撃用
	for (auto& m : meteors_) m->Update();

	// 
	controlGuideSprite_->Update();

	// リザルトスプライトの更新
	result_->Update();

	// 当たり判定
	if (collisionEnabled_) {
		collisionManager_.Update();
	}

	// 狙う弱点マーカーの更新
	UpdateArmTargetMarker();

	leftTargetOuter_->Update();
	leftTargetInner_->Update();
	rightTargetOuter_->Update();
	rightTargetInner_->Update();

	// ボス怒り状態チェック
	if (boss_ && !boss_->IsEnraged()) {
		int hp = boss_->GetHP();
		int maxHp = boss_->GetMaxHp();

		if (maxHp > 0 && hp <= maxHp / 2) {
			boss_->SetEnraged(true);

			if (skybox_) {
				skybox_->SetColor({ 1.0f, 0.3f, 0.3f, 1.0f });
			}
		}
	}


	// パーティクルの更新処理
	ParticleManager::GetInstance()->Update();

	// ポストエフェクトの変更
	ChangePostEffect();

	// -------------------------------------------------------------------- //

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			fade_->Stop();
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:

		if (System::PushKey(DIK_T)) {
			fade_->Start(Fade::Status::FadeOut, 0.6f);
			phase_ = Phase::kFadeOut;
		}

		// ボスが死んでいて、着地済みならタイマー進行
		if (boss_
			&& boss_->GetHP() <= 0
			&& boss_->HasLanded()
			&& endReason_ == EndReason::None) {

			// 撃破後はプレイヤーの射撃を無効化
			if (player_) {
				player_->SetCanShoot(false);
			}

			bossDeathTimer_ += dt;

			// 3秒経ったらリザルトスプライトをスライドイン開始
			if (bossDeathTimer_ >= 3.0f) {
				if (result_) {
					result_->StartSlideIn();
				}
				endReason_ = EndReason::BossDeath;   // 「クリア状態」になっただけ
			}
		} else {
			// ボスが死んでいない or 未着地の時はタイマーリセット
			bossDeathTimer_ = 0.0f;
		}

		// ResultImage がスライド完了したら SPACE でフェードアウト開始
		if (result_ && result_->IsSlideFinished()) {
			if (System::TriggerKey(DIK_SPACE) || System::TriggerKey(DIK_RETURN)) {

				// フェードアウト開始
				fade_->Start(Fade::Status::FadeOut, 0.6f);
				phase_ = Phase::kFadeOut;
				endReason_ = EndReason::BossDeath;   // ← ボス撃破扱い

				return;
			}
		}

		break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			Fade::SetDefaultOpenModeSlash(false);

			if (endReason_ == EndReason::BossDeath || endReason_ == EndReason::GoTitle) {
				sceneManager_->ChangeScene("TITLE");
			} else if (endReason_ == EndReason::PlayerDeath) {
				sceneManager_->ChangeScene("GAMEOVER");
			}
		}
		break;
	}

	ImGuiDebug();
}

void BossTestScene::Draw() {

	// Skyboxの描画
	skybox_->Draw();
	// 地面オブジェクトの描画
	glassObject_->Draw();

	// -------------------- ゲームオブジェクトシーンの描画 -------------------- //

	// Playerの銃描画
	// gun_->Draw();

	// Bossの描画
	boss_->Draw();
	boss_->HPDraw();

	// Bossのメテオ描画
	for (auto& m : meteors_) m->Draw();

	// Playerは一人称視点なので非描画
	player_->Draw();

	// 
	controlGuideSprite_->Draw();

	// 
	leftTargetOuter_->Draw();
	leftTargetInner_->Draw();
	rightTargetOuter_->Draw();
	rightTargetInner_->Draw();

	// デバッグライン
	// debugLine_.Draw();

	// パーティクル描画
	ParticleManager::GetInstance()->Draw();

	// リザルト
	result_->Draw();

	// ポーズメニュー
	if (pauseMenu_ && pauseMenu_->IsPaused()) {
		pauseMenu_->Draw();
	}

	// --------------------------------------------------------------------//

	if (fade_) { fade_->Draw(); }
}

void BossTestScene::Finalize() {}

void BossTestScene::ImGuiDebug() {

#ifdef USE_IMGUI

	glassObject_->ImGuiDebug("glass");
	camera_->ImGuiDebug();
	player_->ImGuiDebug();
	gun_->ImGuiDebug("gun");
	boss_->ImGuiDebug();

	controlGuideSprite_->ImGuiDebug();

	ImGui::Begin("BossTestScene");

	ImGui::Checkbox("isCameraFollowPlayer", &isCameraFollowPlayer_);

	{
		const char* postEffectItems[] = {
			"Auto (Low HP Vignette)", // 0
			"None",                   // 1
			"Grayscale",              // 2
			"Vignetting",             // 3
			"Smoothing (BoxFilter)",  // 4
			"Gaussinan Filter",       // 5
			"Radial Blur",            // 6
			"Random",                 // 7
			"Outline",                // 8
			"Glitch",                 // 9
			"Pixelation",             // 10
			"Chromatic Aberration",   // 11
			"VHS Noise",              // 12
			"Color Inversion",        // 13
		};

		int current = static_cast<int>(postEffectDebugMode_);
		if (ImGui::Combo("Post Effect", &current,
			postEffectItems, IM_ARRAYSIZE(postEffectItems))) {
			postEffectDebugMode_ = static_cast<PostEffectDebugMode>(current);
		}
	}

	ImGui::Separator();

	ImGui::End();

#endif // _DEBUG
}

void BossTestScene::UpdateCamera(float dt) {

	if (koActive_) {
		return;
	}

	if (attackManager_ && attackManager_->GetMeteor() && attackManager_->GetMeteor()->IsActive()) {
		return;
	}

	const bool focusBoss = (boss_ && boss_->WantsCameraFocus());

	// フォーカス中だけ player 追従を切る
	isCameraFollowPlayer_ = !focusBoss;

	// =========================
	// 目標カメラ位置を決める
	// =========================
	Vector3 targetCamPos = camera_->GetTranaslate();

	if (isCameraFollowPlayer_) {
		// 通常：プレイヤー追従
		targetCamPos = player_->GetTransform().translate;
	}

	// 現在位置 → 目標位置へ Lerp
	Vector3 curCamPos = camera_->GetTranaslate();
	Vector3 newCamPos = MyMath::Lerp(curCamPos, targetCamPos, cameraPosLerp_);
	camera_->SetTranslate(newCamPos);

	// =========================
	// 目標カメラ回転を決める
	// =========================
	Vector3 targetRot = camera_->GetRotate();

	if (focusBoss) {
		// ボスを向く
		targetRot = CalcLookAtRotation(newCamPos, boss_->GetCameraFocusPos());
	} else if (isCameraFollowPlayer_) {
		// 通常はプレイヤー向き
		targetRot = player_->GetTransform().rotate;
	}

	// 現在回転 → 目標回転へ Lerp
	Vector3 curRot = camera_->GetRotate();
	Vector3 newRot = MyMath::Lerp(curRot, targetRot, cameraRotLerp_);
	camera_->SetRotate(newRot);
}

void BossTestScene::ChangePostEffect() {

	auto* offscreen = System::GetOffscreenRendering();
	if (!offscreen || !player_) {
		return;
	}

	// --- Auto モード：今まで通り「低HPのときだけビネット」 ---
	if (postEffectDebugMode_ == PostEffectDebugMode::Auto) {

		constexpr int LOW_HP_THRESHOLD = 1; // HP1以下
		bool nowLow = player_->IsLowHP(LOW_HP_THRESHOLD);

		if (nowLow && !lowHpVfxOn_) {
			offscreen->SetPostEffect("Vignetting");
			lowHpVfxOn_ = true;
		} else if (!nowLow && lowHpVfxOn_) {
			// 低HPを脱したら元に戻す
			offscreen->SetPostEffect("none");
			lowHpVfxOn_ = false;
		}

		return;
	}

	// --- 手動モード：ImGui で選んだエフェクトを常に適用 ---
	const char* effectName = "none";

	switch (postEffectDebugMode_) {
	case PostEffectDebugMode::None:
		effectName = "none";
		break;
	case PostEffectDebugMode::Grayscale:
		effectName = "Grayscale";
		break;
	case PostEffectDebugMode::Vignetting:
		effectName = "Vignetting";
		break;
	case PostEffectDebugMode::Smoothing:
		effectName = "Smoothing";
		break;
	case PostEffectDebugMode::GaussinanFilter:
		effectName = "GaussinanFilter";
		break;
	case PostEffectDebugMode::RadialBlur:
		effectName = "RadialBlur";
		break;
	case PostEffectDebugMode::Random:
		effectName = "Random";
		break;
	case PostEffectDebugMode::Outline:
		effectName = "Outline";
		break;
	case PostEffectDebugMode::Glitch:
		effectName = "Glitch";
		break;
	case PostEffectDebugMode::Pixel:
		effectName = "Pixel";
		break;
	case PostEffectDebugMode::ChromaticAberration:
		effectName = "ChromaticAberration";
		break;
	case PostEffectDebugMode::VHSNoise:
		effectName = "VHSNoise";
		break;
	case PostEffectDebugMode::ColorInversion:
		effectName = "ColorInversion";
		break;
	default:
		effectName = "none";
		break;
	}

	offscreen->SetPostEffect(effectName);

	// 手動モード中は lowHpVfxOn_ フラグは使わない
	lowHpVfxOn_ = false;
}

void BossTestScene::StartKnockout(int fallSide) {

	// すでにノックアウト中なら何もしない
	if (koActive_) {
		return;
	}

	KnockoutCameraController::Params p;
	p.fallSide = fallSide;

	// ノックアウトカメラ開始
	ko_.Start(camera_.get(), 0.0f, p);
	koActive_ = true;
	koFrozen_ = false;

	// プレイヤー追従カメラを止める
	isCameraFollowPlayer_ = false;

	// シーン終了理由を「プレイヤー死亡」にしておく
	if (endReason_ == EndReason::None) {
		endReason_ = EndReason::PlayerDeath;
	}
}

void BossTestScene::UpdatePlayerDeath(float dt) {

	// --- ノックアウト開始トリガー ---

	// デバッグ用：Kキーで強制ノックアウト
	if (System::TriggerKey(DIK_K)) {
		StartKnockout(+1); // +1 or -1 で倒れる向き指定
	}

	// HP0 で自動ノックアウト
	if (player_ && player_->GetHP() <= 0 && !koActive_) {
		StartKnockout(+1);
	}

	// --- ノックアウトカメラの更新 ---
	if (koActive_ && !koFrozen_) {
		ko_.Update(dt, camera_.get());

		if (ko_.IsDone()) {
			koFrozen_ = true; // ← ここがポイント

			if (fade_ && phase_ == Phase::kMain) {
				fade_->Start(Fade::Status::FadeOut, 0.6f);
				phase_ = Phase::kFadeOut;
				if (endReason_ == EndReason::None) {
					endReason_ = EndReason::PlayerDeath;
				}
			}
		}
	}
}

void BossTestScene::UpdateGun() {

	if (!gun_ || !camera_) return;

	Vector3 camPos = camera_->GetTranaslate();
	Vector3 camRot = camera_->GetRotate();

	float cp = std::cos(camRot.x), sp = std::sin(camRot.x);
	float cy = std::cos(camRot.y), sy = std::sin(camRot.y);

	Vector3 forward = { sy * cp, -sp, cy * cp }; // カメラ前方
	Vector3 right = { cy, 0.0f, -sy };         // カメラ右
	Vector3 up = { 0.0f, 1.0f, 0.0f };      // ワールド上

	Vector3 gunWorldPos =
		camPos
		+ right * gunOffset_.x
		+ up * gunOffset_.y
		+ forward * gunOffset_.z;

	gun_->SetTranslate(gunWorldPos);

	Vector3 gunWorldRot;
	gunWorldRot.x = gunRotOffset_.x + camRot.x * 0.0f;
	gunWorldRot.y = camRot.y + gunRotOffset_.y;
	gunWorldRot.z = gunRotOffset_.z;
	gun_->SetRotate(gunWorldRot);

	gun_->Update();

	// 銃の先端（今はモデルの原点）をプレイヤーに渡す
	if (player_) {
		// もし本当に「銃の先」にしたければ forward に少し足す
		Vector3 muzzle = gun_->GetWorldPosition() + forward * 1.0f; // 1.0f は好みで調整
		player_->SetGunMuzzlePos(muzzle);
	}
}

void BossTestScene::UpdateMeteorControl() {

	if (!attackManager_) return;

	// デバッグ：MでON/OFF
	if (System::TriggerKey(DIK_M)) {
		if (!attackManager_->IsMeteorActive()) attackManager_->StartMeteor();
		else attackManager_->ForceEndMeteor();
	}
}

void BossTestScene::InitIntro() {

	// ゲームフロー状態を Intro に
	flowState_ = GameFlowState::Intro;

	// ゲーム処理停止
	collisionEnabled_ = false;
	player_->SetControlEnabled(false);
	boss_->SetCombatEnabled(false);

	// ボスを Play位置の真上へ
	Vector3 pos = bossPlayPos_;
	pos.y += bossStartHeight_;
	boss_->SetTranslate(pos);

	landingTriggered_ = false;

	// ---- イントロ演出の初期値を毎回リセット ---- //
	introPhase_ = IntroPhase::CamIn;
	introCamLerp_ = 0.0f;
	landingTimer_ = 0.0f;
	landingTriggered_ = false;

	// 現在のカメラ状態を保存（これを基準に CamIn する）
	introSavedCamPos_ = camera_->GetTranaslate();
	introSavedCamRot_ = camera_->GetRotate();
}

void BossTestScene::UpdateArmTargetMarker() {

	if (!boss_ || !camera_) return;

	// 共通取得
	const int leftHits = boss_->GetLeftHitCount();
	const int rightHits = boss_->GetRightHitCount();
	const int maxHits = boss_->GetMaxHitCount();

	const bool isLeftAttack = boss_->IsLeftArmAttacking();      // 片手(左)
	const bool isRightAttack = boss_->IsRightArmAttacking();     // 片手(右)
	const bool isBothAttack = boss_->IsBothHandsAttacking();    // 両手

	// ViewProj
	Matrix4x4 view = camera_->GetViewMatrix();
	Matrix4x4 proj = camera_->GetProjectionMatrix();
	Matrix4x4 vp = MyMath::Multiply(view, proj);

	auto projectToScreen = [&](const Vector3& worldPos, Vector2& outScreen) -> bool {

		// wチェック（0だと Transform 内 assert になるので弾く）
		float w =
			worldPos.x * vp.m[0][3] +
			worldPos.y * vp.m[1][3] +
			worldPos.z * vp.m[2][3] +
			vp.m[3][3];

		if (std::fabs(w) < 1e-6f) return false;

		Vector3 ndc = MyMath::Transform(worldPos, vp);

		if (ndc.z <= 0.0f || ndc.z >= 1.0f) return false;

		constexpr float SCREEN_W = 1280.0f;
		constexpr float SCREEN_H = 720.0f;

		outScreen.x = (ndc.x * 0.5f + 0.5f) * SCREEN_W;
		outScreen.y = (-ndc.y * 0.5f + 0.5f) * SCREEN_H;
		return true;
		};

	auto hide = [](std::unique_ptr<Sprite>& o, std::unique_ptr<Sprite>& i) {
		if (!o || !i) return;
		o->SetColor({ 1,1,1,0 });
		i->SetColor({ 1,1,1,0 });
		};

	// まず全部消しておく
	hide(leftTargetOuter_, leftTargetInner_);
	hide(rightTargetOuter_, rightTargetInner_);

	if (boss_->IsRetreating()) {
		return;
	}

	// ---- 左手ターゲット表示条件 ---- //
	bool showLeft = false;
	if (boss_->IsChargeActive()) {
		showLeft = boss_->IsChargeTargetLeft() && (leftHits < maxHits);
	} else {
		showLeft =
			(isLeftAttack && leftHits < maxHits) ||
			(isBothAttack && leftHits < maxHits);
	}

	if (showLeft) {
		Vector2 screen;
		if (projectToScreen(boss_->GetLeftHandWorldPos(), screen)) {

			// シェイク中なら少しランダムにずらす（時間とともに減衰）
			if (leftTargetShakeTime_ > 0.0f) {
				float t = leftTargetShakeTime_ / targetShakeDuration_;
				float amp = targetShakeAmplitude_ * t;
				float ox = MyMath::Rand(-amp, amp);
				float oy = MyMath::Rand(-amp, amp);
				screen.x += ox;
				screen.y += oy;
			}

			leftTargetOuter_->SetPosition(screen);
			leftTargetInner_->SetPosition(screen);
			leftTargetOuter_->SetColor({ 1,1,1,1 });
			leftTargetInner_->SetColor({ 1,1,1,1 });
		}
	} else {
		// 表示しない時は透明にして残像を消す
		leftTargetOuter_->SetColor({ 1, 1, 1, 0 });
		leftTargetInner_->SetColor({ 1, 1, 1, 0 });
	}

	// ---- 右手ターゲット表示条件 ---- //	
	bool showRight = false;
	if (boss_->IsChargeActive()) {
		showRight = !boss_->IsChargeTargetLeft() && (rightHits < maxHits);
	} else {
		showRight =
			(isRightAttack && rightHits < maxHits) ||
			(isBothAttack && rightHits < maxHits);
	}

	if (showRight) {
		Vector2 screen;
		if (projectToScreen(boss_->GetRightHandWorldPos(), screen)) {

			if (rightTargetShakeTime_ > 0.0f) {
				float t = rightTargetShakeTime_ / targetShakeDuration_;
				float amp = targetShakeAmplitude_ * t;
				float ox = MyMath::Rand(-amp, amp);
				float oy = MyMath::Rand(-amp, amp);
				screen.x += ox;
				screen.y += oy;
			}

			rightTargetOuter_->SetPosition(screen);
			rightTargetInner_->SetPosition(screen);
			rightTargetOuter_->SetColor({ 1,1,1,1 });
			rightTargetInner_->SetColor({ 1,1,1,1 });
		}
	} else {
		// 表示しない時は透明にして残像を消す
		rightTargetOuter_->SetColor({ 1, 1, 1, 0 });
		rightTargetInner_->SetColor({ 1, 1, 1, 0 });
	}
}

void BossTestScene::LineTarget() {

	// 両腕と胴体を結ぶラインを追加（これは今まで通りでOK）
	if (boss_) {
		Object3d* body = boss_->GetBody();
		Object3d* leftArm = boss_->GetLeftArm();
		Object3d* rightArm = boss_->GetRightArm();

		if (body) {
			const Vector3 bodyPos = body->GetWorldPosition();

			// 左腕 ↔ 胴体（赤）
			if (leftArm) {
				Vector3 leftPos = leftArm->GetWorldPosition();
				debugLine_.AddLine(leftPos, bodyPos, { 1.0f, 0.1f, 0.1f, 1.0f });
			}

			// 右腕 ↔ 胴体（青）
			if (rightArm) {
				Vector3 rightPos = rightArm->GetWorldPosition();
				debugLine_.AddLine(rightPos, bodyPos, { 0.2f, 0.4f, 1.0f, 1.0f });
			}
		}
	}

	// ===== ここから「全部の当たり判定AABB」を描画 =====

	// CollisionManager から AABB 一覧をもらう
	std::vector<CollisionManager::DebugAABBInfo> infos;
	collisionManager_.CollectDebugAABBs(infos);

	for (const auto& info : infos) {

		// レイヤーごとに色を変える
		Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

		switch (info.layer) {
		case CollisionLayer::Player:
			color = { 0.0f, 1.0f, 0.0f, 1.0f }; // 緑
			break;
		case CollisionLayer::Enemy:
			color = { 1.0f, 0.0f, 0.0f, 1.0f }; // 赤
			break;
		case CollisionLayer::PlayerBullet:
			color = { 0.0f, 1.0f, 1.0f, 1.0f }; // シアン
			break;
		case CollisionLayer::EnemyBullet:
			color = { 1.0f, 0.0f, 1.0f, 1.0f }; // マゼンタ
			break;
		case CollisionLayer::Environment:
		default:
			color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白
			break;
		}

		// AABB を線で描画（AddAABBLines は BossTestScene.h のやつ）
		AddAABBLines(debugLine_, info.box, color);
	}
}

Vector3 BossTestScene::CalcLookAtRotation(const Vector3& camPos, const Vector3& targetPos) {

	Vector3 dir = targetPos - camPos;
	dir = MyMath::Normalize(dir);

	Vector3 rot{};
	rot.x = std::atan2(-dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z)); // pitch
	rot.y = std::atan2(dir.x, dir.z);                                     // yaw
	rot.z = 0.0f;
	return rot;
}

void BossTestScene::UpdateIntro(float dt) {

	// メテオのパラメータをそのまま流用（同じ感じにしたいならこれが一番）
	const auto& mp = attackManager_->GetMeteor()->GetParams();
	Vector3 playerPos = player_->GetTransform().translate;

	// メテオと同じ「目標カメラ」
	Vector3 targetPos = playerPos + mp.camOffset;
	Vector3 targetRot = introSavedCamRot_;
	targetRot.x = mp.pitchUp; // 上向き角

	switch (introPhase_) {
	case IntroPhase::CamIn:
	{

		introCamLerp_ = std::min(1.0f, introCamLerp_ + dt / mp.camIntroTime);
		camera_->SetTranslate(MyMath::Lerp(introSavedCamPos_, targetPos, introCamLerp_));
		camera_->SetRotate(MyMath::Lerp(introSavedCamRot_, targetRot, introCamLerp_));

		if (introCamLerp_ >= 1.0f) {
			introPhase_ = IntroPhase::Falling;
		}
		break;
	}

	case IntroPhase::Falling:
	{

		// --- ボス落下 ---
		Vector3 bossPos = boss_->GetTranslate();
		bossPos.y -= bossFallSpeed_ * dt;

		bool landed = false;
		if (bossPos.y <= bossPlayPos_.y) {
			bossPos.y = bossPlayPos_.y;
			landed = true;
		}
		boss_->SetTranslate(bossPos);

		// 
		if (!landed) {
			// 着地前：今まで通り（メテオ式）
			camera_->SetTranslate(targetPos);
		} else {
			// 着地後：プレイヤー位置へ寄せる（ズレ防止）
			Vector3 cur = camera_->GetTranaslate();
			Vector3 goal = player_->GetTranslate();
			camera_->SetTranslate(MyMath::Lerp(cur, goal, 0.08f));
		}

		// --- 着地処理 ---
		if (landed) {
			if (!landingTriggered_) {
				landingTriggered_ = true;
				landingTimer_ = 0.0f;
				camera_->StartShake(CameraShakeType::Large);
			}

			landingTimer_ += dt;
			if (landingTimer_ >= landingWaitTime_) {
				introPhase_ = IntroPhase::CamOut;
				introCamLerp_ = 0.0f;

				// CamOut の開始点を保存
				introSavedCamPos_ = camera_->GetTranaslate();
				introSavedCamRot_ = camera_->GetRotate();
			}
		}

		// --- 回転（ボス注視） ---
		const Vector3 camPos = camera_->GetTranaslate();
		Vector3 dir = MyMath::Normalize(bossPos - camPos);

		Vector3 lookRot{};
		lookRot.x = std::atan2(-dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));
		lookRot.y = std::atan2(dir.x, dir.z);
		lookRot.z = 0.0f;

		Vector3 curRot = camera_->GetRotate();
		camera_->SetRotate(MyMath::Lerp(curRot, lookRot, 0.15f));

		break;
	}


	case IntroPhase::CamOut:
	{

		// メテオのOutroと同じ：元のカメラへ戻す
		introCamLerp_ = std::min(1.0f, introCamLerp_ + dt / mp.camOutroTime);

		Vector3 curPos = camera_->GetTranaslate();
		Vector3 curRot = camera_->GetRotate();

		camera_->SetTranslate(playCameraPos_);
		camera_->SetRotate(playCameraRot_);

		if (introCamLerp_ >= 1.0f) {
			BeginPlay();
		}
		break;
	}
	}
}

void BossTestScene::BeginPlay() {

	// 
	flowState_ = GameFlowState::Play;

	collisionEnabled_ = true;
	player_->SetControlEnabled(true);
	boss_->SetCombatEnabled(true);
}