#include "BossTestScene.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include "Engine/Base/System/System.h"

#include <memory>
#include <vector>

class BossTestScene::FadeInState : public BossTestScene::SceneState {

public:

	void Enter(BossTestScene& scene) override {
		scene.fadeOutRequested_ = false;
		scene.introFinished_ = false;
	}

	void Update(BossTestScene& scene, float dt) override {
		(void)dt;

		scene.InitIntro();
		scene.ChangeToIntro();
	}

	const char* GetName() const override {
		return "FadeInState";
	}
};

class BossTestScene::IntroState : public BossTestScene::SceneState {

public:

	void Enter(BossTestScene& scene) override {
		scene.introFinished_ = false;
	}

	void Update(BossTestScene& scene, float dt) override {

		scene.UpdateIntro(dt);

		// 見た目の更新だけはやっておく
		if (scene.camera_) {
			scene.camera_->Update();
		}

		if (scene.skybox_) {
			scene.skybox_->Update();
		}

		if (scene.glassObject_) {
			scene.glassObject_->Update();
		}

		if (scene.player_) {
			scene.player_->Update();
		}

		if (scene.boss_) {
			scene.boss_->Update();
		}

		scene.ImGuiDebug();

		// BeginPlay() が呼ばれると introFinished_ が true になる
		if (scene.introFinished_) {
			scene.ChangeToPlay();
		}
	}

	const char* GetName() const override {
		return "IntroState";
	}
};

class BossTestScene::PlayState : public BossTestScene::SceneState {

public:

	void Enter(BossTestScene& scene) override {
		scene.fadeOutRequested_ = false;
	}

	void Update(BossTestScene& scene, float dt) override {

		scene.UpdatePlay(dt);

		// UpdatePlay 内でフェードアウト要求が出たら State を切り替える
		if (scene.fadeOutRequested_) {
			scene.ChangeToFadeOut();
		}
	}

	const char* GetName() const override {
		return "PlayState";
	}
};

class BossTestScene::FadeOutState : public BossTestScene::SceneState {

public:

	void Enter(BossTestScene& scene) override {
		scene.fadeOutRequested_ = false;
	}

	void Update(BossTestScene& scene, float dt) override {
		(void)dt;

		if (scene.endReason_ == EndReason::BossDeath || scene.endReason_ == EndReason::GoTitle) {
			scene.sceneManager_->ChangeScene("TITLE");
		} else if (scene.endReason_ == EndReason::PlayerDeath) {
			scene.sceneManager_->ChangeScene("GAMEOVER");
		}
	}

	const char* GetName() const override {
		return "FadeOutState";
	}
};

void BossTestScene::ChangeToFadeIn() {
	ChangeState(std::unique_ptr<SceneState>(new FadeInState()));
}

void BossTestScene::ChangeToIntro() {
	ChangeState(std::unique_ptr<SceneState>(new IntroState()));
}

void BossTestScene::ChangeToPlay() {
	ChangeState(std::unique_ptr<SceneState>(new PlayState()));
}

void BossTestScene::ChangeToFadeOut() {
	ChangeState(std::unique_ptr<SceneState>(new FadeOutState()));
}

void BossTestScene::Init() {

	// カメラ
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	// camera_->SetTranslate({ 0.0f, 0.0f, -30.0f });
	KomEngine::System::GetParticleManager()->SetCamera(camera_.get());

	// Skybox
	skybox_ = std::make_unique<Skybox>();
	skybox_->Init("./Resources/images/blackCube.dds");
	skybox_->SetDefaultCamera(camera_.get());

	// デバッグライン
	debugLine_.Init(2048, BlendType::BLEND_ALPHA);
	debugLine_.SetCamera(camera_.get());

	// 地面
	glassObject_ = std::make_unique<Object3d>();
	glassObject_->Init("object3d_gridFloor", BlendType::BLEND_NONE);
	glassObject_->SetModel("ground.obj");
	glassObject_->SetDefaultCamera(camera_.get());
	glassObject_->SetTranslate({ 0.0f, -5.0f, 0.0f });
	glassObject_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });


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

	// ミサイル予告用マーカー
	missileTelegraphMarkers_.clear();
	missileTelegraphMarkers_.reserve(4);

	for (int i = 0; i < 4; ++i) {
		auto sp = std::make_unique<Sprite>();
		sp->Init("./Resources/images/outer.png", BlendType::BLEND_ALPHA);
		sp->SetAnchorPoint({ 0.5f, 0.5f });
		sp->SetSize({ 80.0f, 80.0f });
		sp->SetColor({ 1,1,1,0 }); // 初期は非表示
		missileTelegraphMarkers_.push_back(std::move(sp));
	}

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
	//for (auto& m : meteors_) {
	//	// collisionManager_.Register(m.get());
	//}

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
	//for (auto& m : missiles_) {
	//	// collisionManager_.Register(m.get());
	//}

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
	controlGuideSprite_->Init("./Resources/images/mouseLeftClick.png", BlendType::BLEND_ALPHA);
	controlGuideSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	controlGuideSprite_->SetSize(controlGuide1Size_);
	controlGuideSprite_->SetPosition(controlGuide1Pos_);

	controlGuideSprite2_ = std::make_unique<Sprite>();
	controlGuideSprite2_->Init("./Resources/images/mouseLightClickWithText.png", BlendType::BLEND_ALPHA);
	controlGuideSprite2_->SetAnchorPoint({ 0.5f, 0.5f });
	controlGuideSprite2_->SetSize(controlGuide2Size_);
	controlGuideSprite2_->SetPosition(controlGuide2Pos_);

	// 
	toPauseSpr_ = std::make_unique<Sprite>();
	toPauseSpr_->Init("./Resources/images/escape.png", BlendType::BLEND_ALPHA);
	toPauseSpr_->SetAnchorPoint({ 0.5f, 0.5f });
	toPauseSpr_->SetSize(toPauseSize_);
	toPauseSpr_->SetPosition(toPausePos_);

	// チャージ説明用ゲージ
	chargeGaugeFrameSpr_ = std::make_unique<Sprite>();
	chargeGaugeFrameSpr_->Init("./Resources/images/ChargeGueage.png", BlendType::BLEND_ALPHA);
	chargeGaugeFrameSpr_->SetAnchorPoint({ 0.5f, 0.5f });
	chargeGaugeFrameSpr_->SetSize(chargeGaugeFrameSize_);
	chargeGaugeFrameSpr_->SetPosition(chargeGaugePos_);
	chargeGaugeFrameSpr_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	// チャージ説明用ゲージの中身
	chargeGaugeFillSpr_ = std::make_unique<Sprite>();
	chargeGaugeFillSpr_->Init("./Resources/images/ChargeBlueGueage.png", BlendType::BLEND_ALPHA);
	chargeGaugeFillSpr_->SetAnchorPoint({ 0.0f, 0.5f });
	chargeGaugeFillSpr_->SetSize({ 0.0f, chargeGaugeFillBaseSize_.y });
	chargeGaugeFillSpr_->SetPosition({
		chargeGaugePos_.x + chargeGaugeFillOffset_.x,
		chargeGaugePos_.y + chargeGaugeFillOffset_.y });
	chargeGaugeFillSpr_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	// 
	leftClickOverlaySpr_ = std::make_unique<Sprite>();
	leftClickOverlaySpr_->Init("./Resources/images/mouseLeftClickRed.png", BlendType::BLEND_ALPHA);
	leftClickOverlaySpr_->SetAnchorPoint({ 0.5f, 0.5f });
	leftClickOverlaySpr_->SetSize(leftClickOverlaySize_);
	leftClickOverlaySpr_->SetPosition({
		controlGuide1Pos_.x + leftClickOverlayOffset_.x,
		controlGuide1Pos_.y + leftClickOverlayOffset_.y });
	leftClickOverlaySpr_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

	// ボスイントロ後のキラーン演出用
	bossIntroGlintSprite_ = std::make_unique<Sprite>();
	bossIntroGlintSprite_->Init("./Resources/images/moonLight.png", BlendType::BLEND_ADD);
	bossIntroGlintSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	bossIntroGlintSprite_->SetSize({ 0.0f, 0.0f });
	bossIntroGlintSprite_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

	// パーティクル
	auto* pm = KomEngine::System::GetParticleManager();
	pm->Init(BlendType::BLEND_ADD);

	pm->CreateParticleGeoup("hit", "./Resources/images/circle2.png", "a");
	pm->CreateParticleGeoup("explosion", "./Resources/images/circle2.png", "a");
	pm->CreateParticleGeoup("ring", "./Resources/images/gradationLine.png", "ring");
	pm->CreateParticleGeoup("cylinder", "./Resources/images/gradationLine.png", "cylinder");
	pm->CreateParticleGeoup("moonLight", "./Resources/images/moonLight.png", "moonLight");
	pm->CreateParticleGeoup("ribbon", "./Resources/images/moonLight.png", "ribbon");
	pm->CreateParticleGeoup("dust", "./Resources/images/dust.png", "a");
	pm->CreateParticleGeoup("muzzle", "./Resources/images/circle2.png", "a");
	pm->CreateParticleGeoup("trail", "./Resources/images/circle.png", "a");
	pm->CreateParticleGeoup("charge_core", "./Resources/images/circle2.png", "a");
	pm->CreateParticleGeoup("charge_pulse", "./Resources/images/gradationLine.png", "ring");
	pm->CreateParticleGeoup("charge_pulse", "./Resources/images/gradationLine.png", "a");
	pm->CreateParticleGeoup("player_charge_line", "./Resources/images/streak.png", "a");
	pm->CreateParticleGeoup("charge_aura", "./Resources/images/circle2.png", "a");
	pm->CreateParticleGeoup("missile_flame", "./Resources/images/circle2.png", "a");

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

	particleEditor_.Init();
	particleEditor_.SetEnabled(false);
	showParticleEditor_ = false;

	ChangeToFadeIn();

	// KomEngine::System::GetOffscreenRendering()->SetPostEffect("Bloom");
}

void BossTestScene::Update() {

	const float dt = KomEngine::System::GetDeltaTime();

	markerAnimTimer_ += dt;

	if (state_) {
		state_->Update(*this, dt);
	}
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
	// メテオの警戒表示
	if (attackManager_ && attackManager_->GetMeteor()) {
		attackManager_->GetMeteor()->Draw();
	}

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
	// controlGuideSprite2_->Draw();

	if (player_ && player_->IsCharging()) {
		if (chargeGaugeFillSpr_) {
			chargeGaugeFillSpr_->Draw();
		}
		if (chargeGaugeFrameSpr_) {
			chargeGaugeFrameSpr_->Draw();
		}
	}

	if (leftClickOverlaySpr_) {
		leftClickOverlaySpr_->Draw();
	}

	const bool isClearUi = (clearSequenceStarted_ || (result_ && result_->IsSlideFinished()));

	if (!isClearUi) {
		controlGuideSprite_->Draw();
		if (leftClickOverlaySpr_) {
			leftClickOverlaySpr_->Draw();
		}
		toPauseSpr_->Draw();
	}

	/*if (bossIntroGlintActive_ && bossIntroGlintSprite_) {
		bossIntroGlintSprite_->Draw();
	}*/

	// 
	leftTargetOuter_->Draw();
	leftTargetInner_->Draw();
	rightTargetOuter_->Draw();
	rightTargetInner_->Draw();

	for (auto& sp : missileTelegraphMarkers_) {
		if (sp) { sp->Draw(); }
	}

	// デバッグライン
	// debugLine_.Draw();

	// パーティクル描画
	KomEngine::System::GetParticleManager()->Draw();

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
			"Bloom",                  // 14
			"Hex Barrier",            // 15
		};

		int current = static_cast<int>(postEffectDebugMode_);
		if (ImGui::Combo("Post Effect", &current,
			postEffectItems, IM_ARRAYSIZE(postEffectItems))) {
			postEffectDebugMode_ = static_cast<PostEffectDebugMode>(current);
		}
	}

	ImGui::Separator();

	ImGui::Text("Guide UI Layout");

	ImGui::DragFloat2("Guide1 Pos", &controlGuide1Pos_.x, 1.0f);
	ImGui::DragFloat2("Guide1 Size", &controlGuide1Size_.x, 1.0f, 1.0f, 2000.0f);

	ImGui::DragFloat2("Guide2 Pos", &controlGuide2Pos_.x, 1.0f);
	ImGui::DragFloat2("Guide2 Size", &controlGuide2Size_.x, 1.0f, 1.0f, 2000.0f);

	ImGui::DragFloat2("Pause Pos", &toPausePos_.x, 1.0f);
	ImGui::DragFloat2("Pause Size", &toPauseSize_.x, 1.0f, 1.0f, 2000.0f);

	ImGui::Separator();
	ImGui::Text("Charge Gauge UI");

	ImGui::DragFloat2("Gauge Pos", &chargeGaugePos_.x, 1.0f);
	ImGui::DragFloat2("Gauge Frame Size", &chargeGaugeFrameSize_.x, 1.0f, 1.0f, 2000.0f);
	ImGui::DragFloat2("Gauge Fill Size", &chargeGaugeFillBaseSize_.x, 1.0f, 1.0f, 2000.0f);
	ImGui::DragFloat2("Gauge Fill Offset", &chargeGaugeFillOffset_.x, 1.0f);

	ImGui::DragFloat("Gauge Max Time", &chargeGaugeMaxTime_, 0.01f, 0.1f, 10.0f);
	ImGui::Text("Gauge Timer: %.2f", chargeGaugeTimer_);

	ImGui::Checkbox("Show Particle Editor", &showParticleEditor_);
	particleEditor_.SetEnabled(showParticleEditor_);

	ImGui::End();

	if (showParticleEditor_) {
		particleEditor_.DrawImGui();
	}

#endif // _DEBUG
}

void BossTestScene::ChangeState(std::unique_ptr<SceneState> nextState) {

	if (state_) {
		state_->Exit(*this);
	}

	state_ = std::move(nextState);

	if (state_) {
		state_->Enter(*this);
	}
}

void BossTestScene::RequestFadeOut(EndReason reason) {

	if (endReason_ == EndReason::None) {
		endReason_ = reason;
	}

	fadeOutRequested_ = true;
}


void BossTestScene::UpdatePlay(float dt) {

	// ---------------- Pause (Play中だけ) ---------------- //
	const bool canPause = !(result_ && result_->IsSlideFinished());

	// ポーズしていない時だけ、Scene側のESCでポーズを開く
	if (pauseMenu_ && !pauseMenu_->IsPaused()) {

		if (canPause && KomEngine::System::TriggerKey(DIK_ESCAPE)) {
			pauseMenu_->SetPaused(true);
			KomEngine::System::GetInput()->SetMouseCenterLock(false);

			// 開いた瞬間にPauseMenu側のESC判定まで走らせない
			ImGuiDebug();
			return;
		}
	}

	// ポーズ中だけ、PauseMenu側の更新を有効にする
	if (pauseMenu_ && pauseMenu_->IsPaused()) {

		KomEngine::System::GetInput()->SetMouseCenterLock(false);

		const auto r = pauseMenu_->Update(dt);

		if (r == PauseMenu::Result::Resume) {
			pauseMenu_->SetPaused(false);

			// ポーズを閉じてもカーソルを中央固定しない
			KomEngine::System::GetInput()->SetMouseCenterLock(false);

		} else if (r == PauseMenu::Result::GoTitle) {

			pauseMenu_->SetPaused(false);

			// タイトルへ戻る時もカーソル固定しない
			KomEngine::System::GetInput()->SetMouseCenterLock(false);

			RequestFadeOut(EndReason::GoTitle);

			return;
		}

		ImGuiDebug();
		return;
	}

	UpdateMeteorControl();

	UpdatePlayerDeath(dt);

	UpdateCamera(dt);

	if (KomEngine::System::GetInput()->PushKey(DIK_T)) {
		if (attackManager_) {
			const bool next = !attackManager_->IsDebugPauseAllAttacks();
			attackManager_->SetDebugPauseAllAttacks(next);
		}
	}

	if (KomEngine::System::GetInput()->PushKey(DIK_C)) {
		if (attackManager_ && boss_) {

			bool targetLeft = !boss_->IsLeftArmBroken();
			if (boss_->IsLeftArmBroken() && !boss_->IsRightArmBroken()) {
				targetLeft = false;
			}

			attackManager_->RequestDebugChargeAttack(targetLeft);
		}
	}

	BossAttackManager::UpdateFlags f{};
	f.koActive = koActive_;

	const bool bossAlive = (boss_ && boss_->GetHP() > 0);
	const bool canStartBattle =
		(!playStartPending_) &&
		(!bossIntroGlintActive_);

	f.isMainPhase =
		bossAlive &&
		(endReason_ == EndReason::None) &&
		canStartBattle;

	f.isCameraFollowPlayer = isCameraFollowPlayer_;

	if (attackManager_) {
		attackManager_->Update(dt, f);
	}

	if (leftTargetShakeTime_ > 0.0f) {
		leftTargetShakeTime_ -= dt;
		if (leftTargetShakeTime_ < 0.0f) leftTargetShakeTime_ = 0.0f;
	}

	if (rightTargetShakeTime_ > 0.0f) {
		rightTargetShakeTime_ -= dt;
		if (rightTargetShakeTime_ < 0.0f) rightTargetShakeTime_ = 0.0f;
	}

	if (camera_) {
		camera_->Update();
	}

	if (skybox_) {
		skybox_->Update();
	}

	if (glassObject_) {
		glassObject_->Update();
	}

	debugLine_.Update();

	LineTarget();

	UpdateGun();

	if (player_) {
		player_->Update();
	}
	// 
	UpdateHexBarrier(dt);

	for (auto& h : hpHearts_) {
		if (h) {
			h->Update();
		}
	}

	if (boss_) {
		boss_->Update();
	}

	for (auto& m : meteors_) {
		if (m) {
			m->Update();
		}
	}

	for (auto& m : missiles_) {
		if (m) {
			m->Update();
		}
	}

	if (controlGuideSprite_) {
		controlGuideSprite_->Update();
	}

	if (controlGuideSprite2_) {
		controlGuideSprite2_->Update();
	}

	// -----------------------------
	// チャージ説明用ゲージ更新
	// -----------------------------
	bool isPlayerCharging = (player_ && player_->IsCharging());

	float chargeT = player_ ? player_->GetChargeRatio() : 0.0f;
	chargeT = std::clamp(chargeT, 0.0f, 1.0f);

	chargeGaugeTimer_ = chargeT * chargeGaugeMaxTime_;

	if (isPlayerCharging) {
		POINT pt;
		GetCursorPos(&pt);

		HWND hwnd = KomEngine::System::GetWinApp()->GetHwnd();
		ScreenToClient(hwnd, &pt);

		chargeGaugePos_.x = static_cast<float>(pt.x) + chargeGaugeMouseOffset_.x;
		chargeGaugePos_.y = static_cast<float>(pt.y) + chargeGaugeMouseOffset_.y;

		chargeGaugePos_.x = std::clamp(chargeGaugePos_.x, 100.0f, 1180.0f);
		chargeGaugePos_.y = std::clamp(chargeGaugePos_.y, 40.0f, 680.0f);
	}

	if (chargeGaugeFillSpr_) {
		chargeGaugeFillSpr_->SetSize({
			chargeGaugeFillBaseSize_.x * chargeT,
			chargeGaugeFillBaseSize_.y
			});

		chargeGaugeFillSpr_->SetPosition({
			chargeGaugePos_.x + chargeGaugeFillOffset_.x,
			chargeGaugePos_.y + chargeGaugeFillOffset_.y
			});

		float alpha = isPlayerCharging ? 1.0f : 0.0f;
		chargeGaugeFillSpr_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		chargeGaugeFillSpr_->Update();
	}

	if (chargeGaugeFrameSpr_) {
		chargeGaugeFrameSpr_->SetPosition(chargeGaugePos_);
		chargeGaugeFrameSpr_->SetSize(chargeGaugeFrameSize_);

		float alpha = isPlayerCharging ? 1.0f : 0.0f;
		chargeGaugeFrameSpr_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		chargeGaugeFrameSpr_->Update();
	}

	if (leftClickOverlaySpr_) {
		leftClickOverlaySpr_->SetPosition({
			controlGuide1Pos_.x + leftClickOverlayOffset_.x,
			controlGuide1Pos_.y + leftClickOverlayOffset_.y
			});
		leftClickOverlaySpr_->SetSize(leftClickOverlaySize_);

		const bool isLeftClickDown = KomEngine::System::GetInput()->PushMouse(0);

		leftClickOverlaySpr_->SetColor({
			1.0f,
			1.0f,
			1.0f,
			isLeftClickDown ? 1.0f : 0.0f
			});

		leftClickOverlaySpr_->Update();
	}

	if (toPauseSpr_) {
		toPauseSpr_->Update();
	}

	if (controlGuideSprite_) {
		controlGuideSprite_->SetPosition(controlGuide1Pos_);
		controlGuideSprite_->SetSize(controlGuide1Size_);
	}

	if (controlGuideSprite2_) {
		controlGuideSprite2_->SetPosition(controlGuide2Pos_);
		controlGuideSprite2_->SetSize(controlGuide2Size_);
	}

	if (toPauseSpr_) {
		toPauseSpr_->SetPosition(toPausePos_);
		toPauseSpr_->SetSize(toPauseSize_);
	}

	if (chargeGaugeFrameSpr_) {
		chargeGaugeFrameSpr_->SetPosition(chargeGaugePos_);
		chargeGaugeFrameSpr_->SetSize(chargeGaugeFrameSize_);
	}

	UpdateBossIntroGlint(dt);

	UpdateClearSequence(dt);

	if (result_) {
		result_->Update();
	}

	if (collisionEnabled_) {
		collisionManager_.Update();
	}

	UpdateArmTargetMarker();
	UpdateMissileTelegraphMarkers();

	if (leftTargetOuter_) leftTargetOuter_->Update();
	if (leftTargetInner_) leftTargetInner_->Update();
	if (rightTargetOuter_) rightTargetOuter_->Update();
	if (rightTargetInner_) rightTargetInner_->Update();

	for (auto& sp : missileTelegraphMarkers_) {
		if (sp) {
			sp->Update();
		}
	}

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

	KomEngine::System::GetParticleManager()->Update();

	ChangePostEffect();

	// -----------------------------
	// PlayState 固有の終了判定
	// -----------------------------

	if (boss_
		&& boss_->GetHP() <= 0
		&& boss_->HasLanded()
		&& endReason_ == EndReason::None) {

		if (player_) {
			player_->SetCanShoot(false);
			player_->SetControlEnabled(false);
		}

		if (!clearSequenceStarted_) {
			StartClearSequence();
		}
	} else {
		bossDeathTimer_ = 0.0f;
	}

	if (result_ && result_->IsSlideFinished()) {
		if (KomEngine::System::TriggerKey(DIK_SPACE) || KomEngine::System::TriggerKey(DIK_RETURN)) {
			RequestFadeOut(EndReason::BossDeath);
			return;
		}
	}

#ifdef USE_IMGUI
	if (KomEngine::System::TriggerKey(DIK_F10)) {
		showParticleEditor_ = !showParticleEditor_;
		particleEditor_.SetEnabled(showParticleEditor_);
	}

	if (showParticleEditor_) {
		particleEditor_.Update();
	}
#endif

	ImGuiDebug();
	BossAttackSelectImGui();
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

	auto* offscreen = KomEngine::System::GetOffscreenRendering();
	if (!offscreen || !player_) {
		return;
	}

	// バリア演出中は最優先
	if (hexBarrierActive_) {
		offscreen->SetPostEffect("HexBarrier");

		offscreen->SetPostEffectParam(
			hexBarrierProgress_,
			hexBarrierAlpha_,
			hexBarrierScale_,
			hexBarrierLineWidth_
		);

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
	case PostEffectDebugMode::Bloom:
		effectName = "Bloom";
		break;
	case PostEffectDebugMode::HexBarrier:
		effectName = "HexBarrier";
		break;
	default:
		effectName = "none";
		break;
	}

	offscreen->SetPostEffect(effectName);
	if (postEffectDebugMode_ == PostEffectDebugMode::HexBarrier) {
		offscreen->SetPostEffectParam(
			1.0f,
			1.0f,
			hexBarrierScale_,
			hexBarrierLineWidth_
		);
	}

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
	if (KomEngine::System::TriggerKey(DIK_K)) {
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

			RequestFadeOut(EndReason::PlayerDeath);
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
	if (KomEngine::System::TriggerKey(DIK_M)) {
		if (!attackManager_->IsMeteorActive()) attackManager_->StartMeteor();
		else attackManager_->ForceEndMeteor();
	}
}

void BossTestScene::InitIntro() {

	introFinished_ = false;

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

	const bool isLeftAttack = boss_->IsLeftArmAttacking();
	const bool isRightAttack = boss_->IsRightArmAttacking();
	const bool isBothAttack = boss_->IsBothHandsAttacking();

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

	// =========================
	// 共通アニメ値
	// =========================
	const bool isChargeMarker = (boss_->IsChargeActive() && boss_->IsChargeCoreActive());

	const float sizeMul = isChargeMarker ? 1.20f : 1.00f;
	const float pulse = 1.0f + 0.08f * std::sin(markerAnimTimer_ * 4.0f);
	const float innerPulse = 1.0f + 0.05f * std::sin(markerAnimTimer_ * 5.5f + 0.7f);

	const float outerSize = 128.0f * sizeMul * pulse;
	const float innerSize = 112.0f * sizeMul * innerPulse;

	const float outerRot = markerAnimTimer_ * (isChargeMarker ? 1.2f : 0.8f);
	const float innerRot = -markerAnimTimer_ * (isChargeMarker ? 2.0f : 1.5f);

	// =========================================================
	// チャージ中はコアをターゲット表示する
	// =========================================================
	if (isChargeMarker) {

		Vector2 screen;
		if (projectToScreen(boss_->GetChargeCoreWorldPos(), screen)) {

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

			leftTargetOuter_->SetSize({ outerSize, outerSize });
			leftTargetInner_->SetSize({ innerSize, innerSize });

			leftTargetOuter_->SetRotation(outerRot);
			leftTargetInner_->SetRotation(innerRot);

			leftTargetOuter_->SetColor({ 1,1,1,1 });
			leftTargetInner_->SetColor({ 1,1,1,1 });
		}

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

			leftTargetOuter_->SetSize({ outerSize, outerSize });
			leftTargetInner_->SetSize({ innerSize, innerSize });

			leftTargetOuter_->SetRotation(outerRot);
			leftTargetInner_->SetRotation(innerRot);

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

			rightTargetOuter_->SetSize({ outerSize, outerSize });
			rightTargetInner_->SetSize({ innerSize, innerSize });

			rightTargetOuter_->SetRotation(outerRot);
			rightTargetInner_->SetRotation(innerRot);

			rightTargetOuter_->SetColor({ 1,1,1,1 });
			rightTargetInner_->SetColor({ 1,1,1,1 });
		}
	} else {
		rightTargetOuter_->SetColor({ 1,1,1,0 });
		rightTargetInner_->SetColor({ 1,1,1,0 });
	}
}

void BossTestScene::UpdateMissileTelegraphMarkers() {

	if (!camera_ || !attackManager_ || !attackManager_->GetMissile()) {
		return;
	}

	auto* missileCtrl = attackManager_->GetMissile();

	// 全消し
	for (auto& sp : missileTelegraphMarkers_) {
		if (sp) {
			sp->SetColor({ 1,1,1,0 });
		}
	}

	// 予告中だけ表示
	if (!missileCtrl->IsTelegraphing()) {
		return;
	}

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

	// 点滅
	float blink = std::sin(markerAnimTimer_ * 10.0f);
	float alpha = (blink > 0.0f) ? 1.0f : 0.2f;

	// 少しだけ脈動
	float pulse = 1.0f + 0.08f * std::sin(markerAnimTimer_ * 8.0f);
	float size = 80.0f * pulse;

	const int count = std::min(
		static_cast<int>(missileTelegraphMarkers_.size()),
		missileCtrl->GetTelegraphCount()
	);

	for (int i = 0; i < count; ++i) {

		Vector3 worldPos;
		if (!missileCtrl->GetTelegraphWorldPos(i, worldPos)) {
			continue;
		}

		Vector2 screen;
		if (!projectToScreen(worldPos, screen)) {
			continue;
		}

		auto& sp = missileTelegraphMarkers_[i];
		if (!sp) {
			continue;
		}

		sp->SetPosition(screen);
		sp->SetSize({ size, size });
		sp->SetColor({ 1.0f, 0.35f, 0.35f, alpha });
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

void BossTestScene::AddFloorGrid() {

	// まずは雰囲気確認用の仮グリッド
	const float y = -4.95f;

	// グリッドの広さ
	const int halfCount = 40;     // 左右に40本ずつ
	const float spacing = 2.0f;   // 1マスの間隔

	// 色
	const Vector4 mainColor = { 0.15f, 0.45f, 1.0f, 0.55f };
	const Vector4 centerColor = { 0.35f, 0.75f, 1.0f, 0.95f };

	const float minPos = -halfCount * spacing;
	const float maxPos = halfCount * spacing;

	for (int i = -halfCount; i <= halfCount; ++i) {
		float p = static_cast<float>(i) * spacing;

		// 真ん中の軸だけ少し強調
		Vector4 color = (i == 0) ? centerColor : mainColor;

		// Z方向へ伸びる線（X一定）
		debugLine_.AddLine(
			{ p, y, minPos },
			{ p, y, maxPos },
			color
		);

		// X方向へ伸びる線（Z一定）
		debugLine_.AddLine(
			{ minPos, y, p },
			{ maxPos, y, p },
			color
		);
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

	introFinished_ = true;

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

void BossTestScene::BossAttackSelectImGui() {

#ifdef USE_IMGUI
	if (attackManager_) {

		ImGui::Separator();
		ImGui::Text("Boss Attack Debug");

		bool fixedMode = attackManager_->IsDebugFixedAttackMode();
		if (ImGui::Checkbox("Fixed Attack Mode", &fixedMode)) {
			attackManager_->SetDebugFixedAttackMode(fixedMode);
		}

		const char* attackNames[] = {
			"Arm Combo",
			"Charge",
			"Meteor",
			"Rush"
		};

		int currentAttack = 0;

		switch (attackManager_->GetDebugFixedAttackBlock()) {
		case BossAttackManager::BossAttackBlock::ArmCombo:
			currentAttack = 0;
			break;
		case BossAttackManager::BossAttackBlock::Charge:
			currentAttack = 1;
			break;
		case BossAttackManager::BossAttackBlock::Meteor:
			currentAttack = 2;
			break;
		case BossAttackManager::BossAttackBlock::Rush:
			currentAttack = 3;
			break;
		}

		if (ImGui::Combo("Fixed Attack", &currentAttack, attackNames, IM_ARRAYSIZE(attackNames))) {

			BossAttackManager::BossAttackBlock selectedBlock =
				BossAttackManager::BossAttackBlock::Rush;

			switch (currentAttack) {
			case 0:
				selectedBlock = BossAttackManager::BossAttackBlock::ArmCombo;
				break;
			case 1:
				selectedBlock = BossAttackManager::BossAttackBlock::Charge;
				break;
			case 2:
				selectedBlock = BossAttackManager::BossAttackBlock::Meteor;
				break;
			case 3:
				selectedBlock = BossAttackManager::BossAttackBlock::Rush;
				break;
			}

			attackManager_->SetDebugFixedAttackBlock(selectedBlock);
		}

		ImGui::Text("Current Fixed Attack: %s",
			attackManager_->GetAttackBlockName(attackManager_->GetDebugFixedAttackBlock()));
	}
#endif
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

void BossTestScene::StartClearSequence() {

	clearSequenceStarted_ = true;
	clearResultStarted_ = false;

	clearSequenceTimer_ = 0.0f;
	clearExplosionTimer_ = 0.0f;
	clearExplosionStep_ = 0;

	if (camera_) {
		camera_->StartShake(CameraShakeType::Large);
	}
}

void BossTestScene::UpdateClearSequence(float dt) {

	if (!clearSequenceStarted_) {
		return;
	}

	clearSequenceTimer_ += dt;
	clearExplosionTimer_ += dt;

	// 連鎖爆発を3段階で出す
	if (clearExplosionStep_ < 3 && clearExplosionTimer_ >= clearExplosionInterval_) {
		clearExplosionTimer_ = 0.0f;
		TriggerClearExplosionStep(clearExplosionStep_);
		++clearExplosionStep_;
	}

	// 少し長めに待ってから結果表示
	if (!clearResultStarted_ && clearSequenceTimer_ >= 2.8f) {
		clearResultStarted_ = true;

		if (result_) {
			result_->StartSlideIn();
		}

		endReason_ = EndReason::BossDeath;
	}
}

void BossTestScene::TriggerClearExplosionStep(int step) {

	if (!boss_) {
		return;
	}

	auto* pm = KomEngine::System::GetParticleManager();
	if (!pm) {
		return;
	}

	Vector3 center = boss_->GetTranslate();
	Vector3 left = boss_->GetLeftHandWorldPos();
	Vector3 right = boss_->GetRightHandWorldPos();

	switch (step) {
	case 0:
		if (pm->Exists("explosion")) pm->Emit("explosion", center, 55);
		if (pm->Exists("hit"))       pm->Emit("hit", center, 24);
		if (pm->Exists("ring"))      pm->Emit("ring", center, 1);
		if (pm->Exists("dust"))      pm->Emit("dust", center, 16);
		break;

	case 1:
		if (pm->Exists("explosion")) {
			pm->Emit("explosion", left, 24);
			pm->Emit("explosion", right, 24);
		}
		if (pm->Exists("hit")) {
			pm->Emit("hit", left, 12);
			pm->Emit("hit", right, 12);
		}
		break;

	case 2:
		if (pm->Exists("explosion")) pm->Emit("explosion", center, 70);
		if (pm->Exists("hit"))       pm->Emit("hit", center, 30);
		if (pm->Exists("dust"))      pm->Emit("dust", center, 26);
		if (pm->Exists("cylinder"))  pm->Emit("cylinder", center, 3);

		if (camera_) {
			camera_->StartShake(CameraShakeType::Large);
		}
		break;
	}
}

void BossTestScene::UpdateHexBarrier(float dt) {

	auto* input = KomEngine::System::GetInput();
	if (!input) {
		return;
	}

	// 
	const bool rightMouseDown = input->PushMouse(1);

	// 押した瞬間にバリア開始
	if (rightMouseDown && !prevRightMouseDownForBarrier_) {
		hexBarrierActive_ = true;
		hexBarrierTimer_ = 0.0f;
		hexBarrierProgress_ = 0.0f;
		hexBarrierAlpha_ = 1.0f;
	}

	prevRightMouseDownForBarrier_ = rightMouseDown;

	if (!hexBarrierActive_) {
		hexBarrierProgress_ = 0.0f;
		hexBarrierAlpha_ = 0.0f;
		return;
	}

	hexBarrierTimer_ += dt;

	// 広がる
	if (hexBarrierTimer_ <= hexBarrierDuration_) {
		float t = hexBarrierTimer_ / hexBarrierDuration_;
		t = std::clamp(t, 0.0f, 1.0f);

		// easeOut
		hexBarrierProgress_ = 1.0f - (1.0f - t) * (1.0f - t);
		hexBarrierAlpha_ = 1.0f;
	}
	// 少し維持
	else if (hexBarrierTimer_ <= hexBarrierDuration_ + hexBarrierHoldTime_) {
		hexBarrierProgress_ = 1.0f;
		hexBarrierAlpha_ = 1.0f;
	}
	// フェードアウト
	else {
		float fadeT =
			(hexBarrierTimer_ - hexBarrierDuration_ - hexBarrierHoldTime_) / hexBarrierFadeTime_;

		fadeT = std::clamp(fadeT, 0.0f, 1.0f);

		hexBarrierProgress_ = 1.0f;
		hexBarrierAlpha_ = 1.0f - fadeT;
	}

	if (hexBarrierTimer_ >= hexBarrierTotalTime_) {
		hexBarrierActive_ = false;
		hexBarrierTimer_ = 0.0f;
		hexBarrierProgress_ = 0.0f;
		hexBarrierAlpha_ = 0.0f;
	}
}