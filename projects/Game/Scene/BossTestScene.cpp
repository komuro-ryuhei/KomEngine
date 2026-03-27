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

	// カメラ
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	// camera_->SetTranslate({ 0.0f, 0.0f, -30.0f });
	System::GetParticleManager()->SetCamera(camera_.get());

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

	// --- フェード初期化（画面サイズは 1280x720）--- //
	phase_ = Phase::kFadeIn;

	// Player
	player_ = std::make_unique<Player>();
	player_->Init(camera_.get());

	// HPハート
	hpHearts_.clear();
	hpHearts_.reserve(playerMaxHp_);
	for (int i = 0; i < playerMaxHp_; ++i) {
		auto sp = std::make_unique<Sprite>();
		sp->Init("./Resources/images/heart.png", BlendType::BLEND_ALPHA);
		sp->SetSize(hpHeartSize_);
		sp->SetAnchorPoint({ 0.0f, 1.0f }); // 左下アンカー
		sp->SetPosition({ hpStartPos_.x + hpHeartInterval_ * i, hpStartPos_.y });
		hpHearts_.push_back(std::move(sp));
	}

	// Playerが持つ銃
	gun_ = std::make_unique<Object3d>();
	gun_->Init(BlendType::BLEND_NONE);
	gun_->SetModel("gun.obj");
	gun_->SetDefaultCamera(camera_.get());

	// Boss
	boss_ = std::make_unique<BossEnemy>();
	boss_->Init(camera_.get());
	boss_->SetTranslate({ 0.0f, 0.0f, 20.0f });
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
		m->SetCollisionManager(&collisionManager_);
		meteors_.push_back(std::move(m));
	}

	// メテオを CollisionManager に登録
	for (auto& m : meteors_) {
		// collisionManager_.Register(m.get());
	}

	// ボスのミサイル
	missiles_.clear();
	missiles_.reserve(4);
	for (int i = 0; i < 4; ++i) {
		auto m = std::make_unique<BossMissile>();
		m->Init(camera_.get());
		m->SetCollisionManager(&collisionManager_);
		missiles_.push_back(std::move(m));
	}

	// ミサイルを CollisionManager に登録
	for (auto& m : missiles_) {
		// collisionManager_.Register(m.get());
	}

	// ボスの攻撃管理
	attackManager_ = std::make_unique<BossAttackManager>();

	BossAttackManager::InitDesc init{};
	init.camera = camera_.get();
	init.player = player_.get();
	init.boss = boss_.get();
	init.meteors = &meteors_;
	init.missiles = &missiles_;

	attackManager_->Init(init);

	// jsonの読み込み
	attackManager_->GetMeteor()->LoadParamsFromJson("Resources/json/bossAttacks.json");
	attackManager_->GetArm()->LoadParamsFromJson("Resources/json/bossAttacks.json");

	// 
	controlGuideSprite_ = std::make_unique<Sprite>();
	controlGuideSprite_->Init("./Resources/images/mouseLeftClickWithText.png", BlendType::BLEND_ALPHA);
	controlGuideSprite_->SetSize({ 256.0f, 280.0f });
	controlGuideSprite_->SetPosition({ 980.0f, 180.0f });

	controlGuideSprite2_ = std::make_unique<Sprite>();
	controlGuideSprite2_->Init("./Resources/images/mouseLightClickWithText.png", BlendType::BLEND_ALPHA);
	controlGuideSprite2_->SetSize({ 256.0f, 280.0f });
	controlGuideSprite2_->SetPosition({ 980.0f, 420.0f });

	// 
	toPauseSpr_ = std::make_unique<Sprite>();
	toPauseSpr_->Init("./Resources/images/toPause.png", BlendType::BLEND_ALPHA);
	toPauseSpr_->SetSize({ 320.0f, 64.0f });
	toPauseSpr_->SetAnchorPoint({ 0.5f, 0.5f });
	toPauseSpr_->SetPosition({ 1100.0f, 100.0f });

	// ボスイントロ後のキラーン演出用
	bossIntroGlintSprite_ = std::make_unique<Sprite>();
	bossIntroGlintSprite_->Init("./Resources/images/moonLight.png", BlendType::BLEND_ADD);
	bossIntroGlintSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	bossIntroGlintSprite_->SetSize({ 0.0f, 0.0f });
	bossIntroGlintSprite_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

	// パーティクル
	auto* pm = System::GetParticleManager();
	pm->Init(BlendType::BLEND_ADD);

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
	pm->CreateParticleGeoup("charge_core", circle2, "a");
	pm->CreateParticleGeoup("charge_pulse", ring, "a");

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
	collisionManager_.AddPairRule(CollisionLayer::Player, CollisionLayer::EnemyMeteor);
	collisionManager_.AddPairRule(CollisionLayer::Player, CollisionLayer::EnemyCharge);
	collisionManager_.AddPairRule(CollisionLayer::Player, CollisionLayer::EnemyMissile);
	
	collisionManager_.AddPairRule(CollisionLayer::PlayerBullet, CollisionLayer::Enemy);
	collisionManager_.AddPairRule(CollisionLayer::PlayerBullet, CollisionLayer::EnemyBullet);
	collisionManager_.AddPairRule(CollisionLayer::PlayerBullet, CollisionLayer::EnemyMeteor);
	collisionManager_.AddPairRule(CollisionLayer::PlayerBullet, CollisionLayer::EnemyMissile);
	collisionManager_.AddPairRule(CollisionLayer::PlayerBullet, CollisionLayer::EnemyCore);
	

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
		phase_ = Phase::kMain;
		InitIntro();
		return;
	}

	if (flowState_ == GameFlowState::Intro) {
		UpdateIntro(dt);

		// 見た目の更新だけはやっておく
		camera_->Update();
		skybox_->Update();
		player_->Update();
		boss_->Update(); // combatEnabled_ が false なら攻撃しない
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

			// ポーズ解除
			pauseMenu_->SetPaused(false);
			System::GetInput()->SetMouseCenterLock(true);

			// フェードアウト開始
			phase_ = Phase::kFadeOut;
			endReason_ = EndReason::GoTitle;

			return;
		}

		ImGuiDebug(); // ポーズ中もデバッグは出す
		return;
	}
	// ---------------------------------------------------

	UpdateMeteorControl();

	// プレイヤー死亡処理
	UpdatePlayerDeath(dt);

	UpdateCamera(dt);

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

	// キラーン中 / 開始待ち中は攻撃を進めない
	const bool bossAlive = (boss_ && boss_->GetHP() > 0);
	const bool canStartBattle =
		(flowState_ == GameFlowState::Play) &&
		(!playStartPending_) &&
		(!bossIntroGlintActive_);

	f.isMainPhase =
		(phase_ == Phase::kMain) &&
		bossAlive &&
		(endReason_ == EndReason::None) &&
		canStartBattle;

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
	// PlayerのHP表示更新
	for (auto& h : hpHearts_) {
		if (h) { h->Update(); }
	}

	// ボス
	boss_->Update();

	// ボスのメテオ攻撃用
	for (auto& m : meteors_) {
		m->Update();
	}

	for (auto& m : missiles_) {
		m->Update();
	}

	// 
	controlGuideSprite_->Update();
	controlGuideSprite2_->Update();

	toPauseSpr_->Update();

	UpdateBossIntroGlint(dt);

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

			if (attackManager_) {
				attackManager_->StartEnragePause(2.5f);
			}

			if (skybox_) {
				skybox_->SetColor({ 1.0f, 0.3f, 0.3f, 1.0f });
			}
		}
	}

	// パーティクルの更新処理
	System::GetParticleManager()->Update();

	// ポストエフェクトの変更
	ChangePostEffect();

	// -------------------------------------------------------------------- //

	switch (phase_) {

	case Phase::kFadeIn:
		phase_ = Phase::kMain;
		break;

	case Phase::kMain:

		if (System::PushKey(DIK_T)) {
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
				endReason_ = EndReason::BossDeath; // 「クリア状態」になっただけ
			}
		} else {
			// ボスが死んでいない or 未着地の時はタイマーリセット
			bossDeathTimer_ = 0.0f;
		}

		// ResultImage がスライド完了したら SPACE でフェードアウト開始
		if (result_ && result_->IsSlideFinished()) {
			if (System::TriggerKey(DIK_SPACE) || System::TriggerKey(DIK_RETURN)) {

				// フェードアウト開始
				phase_ = Phase::kFadeOut;
				endReason_ = EndReason::BossDeath;

				return;
			}
		}

		break;

	case Phase::kFadeOut:

		if (endReason_ == EndReason::BossDeath || endReason_ == EndReason::GoTitle) {
			sceneManager_->ChangeScene("TITLE");
		} else if (endReason_ == EndReason::PlayerDeath) {
			sceneManager_->ChangeScene("GAMEOVER");
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

	// Bossのミサイル描画
	for (auto& m : missiles_) {
		m->Draw();
	}

	// Playerは一人称視点なので非描画
	player_->Draw();

	// HPハート描画
	if (player_) {
		const int hp = std::clamp(player_->GetHP(), 0, playerMaxHp_);
		for (int i = 0; i < hp && i < static_cast<int>(hpHearts_.size()); ++i) {
			hpHearts_[i]->Draw();
		}
	}

	// 
	controlGuideSprite_->Draw();
	controlGuideSprite2_->Draw();

	toPauseSpr_->Draw();

	if (bossIntroGlintActive_ && bossIntroGlintSprite_) {
		bossIntroGlintSprite_->Draw();
	}

	// 
	leftTargetOuter_->Draw();
	leftTargetInner_->Draw();
	rightTargetOuter_->Draw();
	rightTargetInner_->Draw();

	// デバッグライン
	// debugLine_.Draw();

	// パーティクル描画
	System::GetParticleManager()->Draw();

	// リザルト
	result_->Draw();

	// ポーズメニュー
	if (pauseMenu_ && pauseMenu_->IsPaused()) {
		pauseMenu_->Draw();
	}

	// --------------------------------------------------------------------//

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
	controlGuideSprite2_->ImGuiDebug();

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
	const bool focusChargeCore =
		(boss_ && boss_->IsChargeActive() && boss_->IsChargeCoreActive());

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
		// 退避中などは従来通りボスを向く
		targetRot = CalcLookAtRotation(newCamPos, boss_->GetCameraFocusPos());
	} else if (isCameraFollowPlayer_) {
		// 通常はプレイヤー向き
		targetRot = player_->GetTransform().rotate;
	}

	// =========================
	// チャージ中だけコアを見る回転を少し混ぜる
	// =========================
	if (focusChargeCore) {
		chargeLookActive_ = true;
		chargeLookBlend_ += dt * chargeLookInSpeed_;
		if (chargeLookBlend_ > 1.0f) {
			chargeLookBlend_ = 1.0f;
		}
	} else {
		chargeLookActive_ = false;
		chargeLookBlend_ -= dt * chargeLookOutSpeed_;
		if (chargeLookBlend_ < 0.0f) {
			chargeLookBlend_ = 0.0f;
		}
	}

	if (chargeLookBlend_ > 0.0f && boss_) {
		Vector3 chargeLookTarget = boss_->GetChargeCoreWorldPos() + chargeLookOffset_;
		Vector3 chargeRot = CalcLookAtRotation(newCamPos, chargeLookTarget);

		// 少しだけ向ける
		float t = chargeLookBlend_ * 0.55f;
		targetRot = MyMath::Lerp(targetRot, chargeRot, t);
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

	// --- Auto モード：今まで通り「低HPのときだけビネット」 --- //
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

	// --- 手動モード：ImGui で選んだエフェクトを常に適用 --- //
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

	// --- ノックアウト開始トリガー --- //

	// デバッグ用：Kキーで強制ノックアウト
	if (System::TriggerKey(DIK_K)) {
		StartKnockout(+1); // +1 or -1 で倒れる向き指定
	}

	// HP0 で自動ノックアウト
	if (player_ && player_->GetHP() <= 0 && !koActive_) {
		StartKnockout(+1);
	}

	// --- ノックアウトカメラの更新 --- //
	if (koActive_ && !koFrozen_) {
		ko_.Update(dt, camera_.get());

		if (ko_.IsDone()) {
			koFrozen_ = true;

			phase_ = Phase::kFadeOut;
			if (endReason_ == EndReason::None) {
				endReason_ = EndReason::PlayerDeath;
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
	Vector3 right = { cy, 0.0f, -sy };           // カメラ右
	Vector3 up = { 0.0f, 1.0f, 0.0f };           // ワールド上

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
		Vector3 muzzle = gun_->GetWorldPosition() + forward * 1.0f;
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

	const bool isLeftAttack = boss_->IsLeftArmAttacking();   // 片手(左)
	const bool isRightAttack = boss_->IsRightArmAttacking(); // 片手(右)
	const bool isBothAttack = boss_->IsBothHandsAttacking(); // 両手

	// ViewProj
	Matrix4x4 view = camera_->GetViewMatrix();
	Matrix4x4 proj = camera_->GetProjectionMatrix();
	Matrix4x4 vp = MyMath::Multiply(view, proj);

	auto projectToScreen = [&](const Vector3& worldPos, Vector2& outScreen) -> bool {

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

	// =========================================================
	// チャージ中はコアをターゲット表示する
	// =========================================================
	if (boss_->IsChargeActive() && boss_->IsChargeCoreActive()) {

		Vector2 screen;
		if (projectToScreen(boss_->GetChargeCoreWorldPos(), screen)) {

			// チャージ中は左側ターゲットUIをコア用として使う
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

		// 右側は使わないので非表示のまま
		rightTargetOuter_->SetColor({ 1,1,1,0 });
		rightTargetInner_->SetColor({ 1,1,1,0 });
		return;
	}

	// =========================================================
	// 通常時：左手ターゲット
	// =========================================================
	bool showLeft =
		(isLeftAttack && leftHits < maxHits) ||
		(isBothAttack && leftHits < maxHits);

	if (showLeft) {
		Vector2 screen;
		if (projectToScreen(boss_->GetLeftHandWorldPos(), screen)) {

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
		leftTargetOuter_->SetColor({ 1,1,1,0 });
		leftTargetInner_->SetColor({ 1,1,1,0 });
	}

	// =========================================================
	// 通常時：右手ターゲット
	// =========================================================
	bool showRight =
		(isRightAttack && rightHits < maxHits) ||
		(isBothAttack && rightHits < maxHits);

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
		rightTargetOuter_->SetColor({ 1,1,1,0 });
		rightTargetInner_->SetColor({ 1,1,1,0 });
	}
}

void BossTestScene::LineTarget() {

	// 両腕と胴体を結ぶライン
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

	// ===== ここから「全部の当たり判定AABB」を描画 ===== //

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

		// AABB を線で描画
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

	// メテオのパラメータをそのまま流用
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

		// --- ボス落下 --- //
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

		// --- 着地処理 --- //
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

		// --- 回転（ボス注視） --- //
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

	flowState_ = GameFlowState::Play;

	// まだ戦闘開始しない
	collisionEnabled_ = false;

	if (player_) {
		player_->SetControlEnabled(false);
	}
	if (boss_) {
		boss_->SetCombatEnabled(false);
	}

	// キラーンが終わったら開始する
	playStartPending_ = true;
	StartBossIntroGlint();

	// プレイ会用
	if (boss_) {
		// boss_->SetHP(0);
	}
}

void BossTestScene::StartBossIntroGlint() {

	if (!bossIntroGlintSprite_) {
		return;
	}

	bossIntroGlintActive_ = true;
	bossIntroGlintTimer_ = 0.0f;

	bossIntroGlintSprite_->SetSize({ 0.0f, 0.0f });
	bossIntroGlintSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
}

void BossTestScene::UpdateBossIntroGlint(float dt) {

	if (!bossIntroGlintActive_ || !bossIntroGlintSprite_ || !boss_ || !camera_) {
		return;
	}

	bossIntroGlintTimer_ += dt;

	float t = bossIntroGlintTimer_ / bossIntroGlintDuration_;
	if (t >= 1.0f) {
		bossIntroGlintActive_ = false;
		bossIntroGlintSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

		// キラーン終了後に戦闘開始
		if (playStartPending_) {
			playStartPending_ = false;
			collisionEnabled_ = true;

			if (player_) {
				player_->SetControlEnabled(true);
			}
			if (boss_) {
				boss_->SetCombatEnabled(true);
			}
		}

		return;
	}

	// ボス中央より少し上を狙う
	Vector3 worldPos = boss_->GetTranslate() + bossIntroGlintOffset_;

	// ViewProj
	Matrix4x4 view = camera_->GetViewMatrix();
	Matrix4x4 proj = camera_->GetProjectionMatrix();
	Matrix4x4 vp = MyMath::Multiply(view, proj);

	// 画面投影
	float w =
		worldPos.x * vp.m[0][3] +
		worldPos.y * vp.m[1][3] +
		worldPos.z * vp.m[2][3] +
		vp.m[3][3];

	if (std::fabs(w) < 1e-6f) {
		bossIntroGlintSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
		return;
	}

	Vector3 ndc = MyMath::Transform(worldPos, vp);

	if (ndc.z <= 0.0f || ndc.z >= 1.0f) {
		bossIntroGlintSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
		return;
	}

	constexpr float SCREEN_W = 1280.0f;
	constexpr float SCREEN_H = 720.0f;

	Vector2 screen{};
	screen.x = (ndc.x * 0.5f + 0.5f) * SCREEN_W;
	screen.y = (-ndc.y * 0.5f + 0.5f) * SCREEN_H;

	bossIntroGlintSprite_->SetPosition(screen);

	// 最初に一気に広がって、後半で消える
	float scale = 220.0f;
	if (t < 0.25f) {
		scale = 220.0f * (t / 0.25f); // 0 -> 220
	} else {
		float u = (t - 0.25f) / 0.75f;
		scale = 220.0f - 100.0f * u;  // 220 -> 120
	}

	float alpha = 1.0f;
	if (t < 0.2f) {
		alpha = t / 0.2f; // フェードイン
	} else {
		alpha = 1.0f - ((t - 0.2f) / 0.8f); // フェードアウト
	}
	alpha = std::clamp(alpha, 0.0f, 1.0f);

	bossIntroGlintSprite_->SetSize({ scale, scale });
	bossIntroGlintSprite_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
	bossIntroGlintSprite_->Update();
}