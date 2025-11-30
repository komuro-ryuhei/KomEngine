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

	// --- ここでコントローラ初期化 ---
	meteorController_ = std::make_unique<BossMeteorController>();
	meteorController_->Init();
	meteorController_->SetCamera(camera_.get());
	meteorController_->SetPlayer(player_.get());
	meteorController_->SetBoss(boss_.get());
	meteorController_->SetMeteors(&meteors_);

	// JSON読み込み
	meteorController_->LoadParamsFromJson("Resources/json/bossAttacks.json");

	// 腕コントローラ初期化
	armController_ = std::make_unique<BossArmController>();
	armController_->Init(camera_.get(), boss_.get());

	// ボスの剣
	sword_ = std::make_unique<BossSword>();
	sword_->Init(camera_.get());

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


	// グループが既にあれば作らない
	if (!pm->Exists("hit")) {
		pm->CreateParticleGeoup("hit", "./Resources/images/circle2.png", "hit");
	}

	emitter_ = std::make_unique<ParticleEmitter>();
	emitter_->Init("hit", { 0.0f, 0.0f, 10.0f }, 10);

	result_ = std::make_unique<ResultImage>();
	result_->Init();
}

void BossTestScene::Update() {

	const float dt = 1.0f / 60.0f;

	UpdateMeteorControl(dt);

	// 剣攻撃トリガー
	if (System::TriggerKey(DIK_J) && !swordAttack_) {
		swordAttack_ = true;
		swordPhaseT_ = 0.f;
	}

	// ノックアウトカメラの開始（Kキー）
	if (System::TriggerKey(DIK_K)) {
		if (!koActive_) {
			KnockoutCameraController::Params p; p.fallSide = +1;
			ko_.Start(camera_.get(), 0.0f, p);
			koActive_ = true; isCameraFollowPlayer_ = false;
		}
	}

	// ノックアウトカメラの更新
	if (koActive_) {
		ko_.Update(dt, camera_.get());
		if (ko_.IsDone()) {
			koActive_ = false;

			// フェードアウト開始（まだ何も決まってなければプレイヤー死亡扱い）
			if (fade_ && phase_ == Phase::kMain) {
				fade_->Start(Fade::Status::FadeOut, 0.6f);
				phase_ = Phase::kFadeOut;
				if (endReason_ == EndReason::None) {
					endReason_ = EndReason::PlayerDeath;
				}
			}
		}
	}

	// ----------------------- ゲームオブジェクトの更新 ----------------------- //

	// ---- 剣フォーカス中はカメラをターゲットに向ける ---- //

	if (swordCamActive_) {
		Vector3 camPos = camera_->GetTranaslate();
		Vector3 to = swordAimPoint_ - camPos;
		Vector3 dir = MyMath::Normalize(to);
		float pitch = -std::asin(dir.y);
		float yaw = std::atan2(dir.x, dir.z);

		// イントロ補間（向ける）
		if (swordPhaseT_ >= 0.41f && (swordCamIntroT_ < 1.0f)) {
			swordCamIntroT_ += dt / swordCamIntroDur_;
			float t = MyMath::Clamp01(swordCamIntroT_);
			camera_->SetRotate(MyMath::Lerp(swordSavedRot_, Vector3{ pitch, yaw, 0.0f }, t));

			// ★ 向き終わった瞬間にスイープ開始（遅延発動）
			if (t >= 1.0f && swordPendingSweep_ && sword_) {
				swordPendingSweep_ = false;
				sword_->SetScale({ 1.6f,1.6f,1.6f });
				sword_->StartSweep(swordCenter_, swordRight_, swordForward_,
					swordHalfLen_, swordToward_, swordDuration_);
			}
		}

		// アウトロ補間（元に戻す）
		if (!sword_->IsAlive()) {
			swordCamOutroT_ += dt / swordCamOutroDur_;
			float t = MyMath::Clamp01(swordCamOutroT_);
			camera_->SetRotate(MyMath::Lerp(camera_->GetRotate(), swordSavedRot_, t));
			if (t >= 1.0f) swordCamActive_ = false;
		}
	}

	// 腕攻撃がこのフレームでカメラを触っていいかどうか
	bool canArmCam =
		!koActive_
		&& phase_ == Phase::kMain           // フェード中は動かさない
		&& (!meteorController_ || !meteorController_->IsActive())  // メテオ中は触らない
		&& !swordCamActive_                 // 剣カメラ中も触らない
		&& isCameraFollowPlayer_;           // プレイヤー追従中だけ

	if (armController_) {
		armController_->Update(dt, canArmCam);
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

	// Playerの銃の更新
	UpdateGun();
	// Playerの更新()
	player_->Update();

	// ボス
	boss_->Update();
	// ボスのメテオ攻撃用
	for (auto& m : meteors_) m->Update();
	// ボスの剣
	if (sword_) sword_->Update();

	// リザルトスプライトの更新
	result_->Update();

	// 当たり判定の確認
	CheckCollisions();

	// 狙う弱点マーカーの更新
	UpdateArmTargetMarker();

	leftTargetOuter_->Update();
	leftTargetInner_->Update();
	rightTargetOuter_->Update();
	rightTargetInner_->Update();


	// パーティクルの更新処理
	ParticleManager::GetInstance()->Update();

	{
		constexpr int LOW_HP_THRESHOLD = 1; // HP1以下
		bool nowLow = player_->IsLowHP(LOW_HP_THRESHOLD);

		if (nowLow && !lowHpVfxOn_) {
			System::GetOffscreenRendering()->SetPostEffect("Vignetting");
			lowHpVfxOn_ = true;
		} else if (!nowLow && lowHpVfxOn_) {
			// 低HPを脱したら元に戻す
			System::GetOffscreenRendering()->SetPostEffect("none");
			lowHpVfxOn_ = false;
		}
	}

	if (swordAttack_) {

		// 右手を縮める
		if (swordPhaseT_ < 0.4f) {
			swordPhaseT_ += dt;
			float t = std::min(1.f, swordPhaseT_ / 0.4f);
			// 右手のスケールを徐々に0へ（BossEnemy側にsetterが無いなら rightArmScale 直接）
			boss_->SetRightHandScale(MyMath::Lerp(Vector3{ 1,1,1 }, Vector3{ 0,0,0 }, t));
		}
		// 縮みきった直後：スイープ開始
		else if (swordPhaseT_ < 0.41f) {
			swordPhaseT_ = 0.41f;

			Vector3 handPos = boss_->GetRightHandWorldPos();

			// カメラ基底
			Vector3 camPos = camera_->GetTranaslate();
			Vector3 camRot = camera_->GetRotate();
			float cp = std::cos(camRot.x), sp = std::sin(camRot.x);
			float cy = std::cos(camRot.y), sy = std::sin(camRot.y);
			Vector3 forward = { sy * cp, -sp, cy * cp };
			Vector3 right = { cy,   0.0f, -sy };

			// 画面中央より少し右、右手の高さ
			Vector3 center = camPos + forward * 6.0f + right * 3.0f;
			center.y = handPos.y;

			// 右→左に薙ぎ（right を反転）
			swordCenter_ = center;
			swordRight_ = { -right.x,-right.y,-right.z };
			swordForward_ = forward;
			swordHalfLen_ = 10.0f;
			swordToward_ = 2.0f;
			swordDuration_ = 0.5f;

			// ここで即スイープ開始
			sword_->SetScale({ 1.6f,1.6f,1.6f });
			sword_->StartSweep(swordCenter_, swordRight_, swordForward_,
				swordHalfLen_, swordToward_, swordDuration_);

			// カメラ制御フラグは明示的にオフ
			swordCamActive_ = false;
			swordPendingSweep_ = false;
		}

		// 剣が消えたら右手を戻して終了
		else if (!sword_->IsAlive()) {
			swordPhaseT_ += dt;
			float t = std::min(1.f, (swordPhaseT_ - 0.41f) / 0.3f);
			boss_->SetRightHandScale(MyMath::Lerp(Vector3{ 0,0,0 }, Vector3{ 1,1,1 }, t));
			if (t >= 1.f) { swordAttack_ = false; swordPhaseT_ = 0.f; }
		}
	}

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

		// ★ ボスが死んでいて、着地済みならタイマー進行
		if (boss_
			&& boss_->GetHP() <= 0
			&& boss_->HasLanded()
			&& endReason_ == EndReason::None) {

			// ★ 撃破後はプレイヤーの射撃を無効化
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

		// ★ ResultImage がスライド完了したら SPACE でフェードアウト開始
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

			if (endReason_ == EndReason::BossDeath) {
				sceneManager_->ChangeScene("TITLE");   // ★復活
			} else if (endReason_ == EndReason::PlayerDeath) {
				sceneManager_->ChangeScene("GAMEOVER");
			}
		}
		break;
	}

#ifdef USE_IMGUI

	ImGui::Begin("BossTestScene");

	glassObject_->ImGuiDebug("glass");
	camera_->ImGuiDebug();
	player_->ImGuiDebug();
	gun_->ImGuiDebug("gun");
	boss_->ImGuiDebug();

	ImGui::Checkbox("isCameraFollowPlayer", &isCameraFollowPlayer_);

	// ==== ここから攻撃エディタ ==== //

	static const char* attackNames[] = { "Meteor", "Sword", "Arms" };
	int currentIndex = static_cast<int>(currentAttackType_);
	if (ImGui::Combo("Attack", &currentIndex, attackNames, IM_ARRAYSIZE(attackNames))) {
		currentAttackType_ = static_cast<BossAttackType>(currentIndex);
	}

	ImGui::Separator();

	switch (currentAttackType_) {
	case BossAttackType::Meteor:

		// メテオ攻撃パラメータ
		if (meteorController_) {

			auto& p = meteorController_->GetParams();

			ImGui::Text("Meteor Attack Params");
			ImGui::DragFloat("Duration", &p.duration, 0.1f, 0.0f, 60.0f);
			ImGui::DragFloat("SpawnInterval", &p.spawnInterval, 0.01f, 0.05f, 5.0f);
			ImGui::DragFloat("CamIntroTime", &p.camIntroTime, 0.01f, 0.0f, 5.0f);
			ImGui::DragFloat("CamOutroTime", &p.camOutroTime, 0.01f, 0.0f, 5.0f);
			ImGui::DragFloat3("CamOffset", &p.camOffset.x, 0.1f);
			ImGui::DragFloat("PitchUp", &p.pitchUp, 0.01f, -1.57f, 0.0f);

			ImGui::Separator();

			// デフォルトに戻すボタン
			if (ImGui::Button("Reset to Default")) {
				p.ResetDefault();
			}

			// 保存ボタン
			if (ImGui::Button("Save Meteor Params")) {
				meteorController_->SaveParamsToJson("Resources/json/bossAttacks.json");
			}
		}
		break;

	case BossAttackType::Sword:
		// ここに剣攻撃のスピード/長さなどのパラメータ ImGui を後で追加
		break;

	case BossAttackType::Arms:
		// 腕攻撃関連のクールタイム・速度などを後で追加
		break;
	}

	ImGui::End();

#endif // _DEBUG

}

void BossTestScene::Draw() {

	// Skyboxの描画
	skybox_->Draw();
	// 地面オブジェクトの描画
	glassObject_->Draw();

	// -------------------- ゲームオブジェクトシーンの描画 -------------------- //

	// Playerの銃描画
	gun_->Draw();

	// Bossの描画
	boss_->Draw();
	boss_->HPDraw();

	// Bossのメテオ描画
	for (auto& m : meteors_) m->Draw();
	// Bossの剣描画
	if (sword_) sword_->Draw();

	// Playerは一人称視点なので非描画
	player_->Draw();

	// 
	leftTargetOuter_->Draw();
	leftTargetInner_->Draw();
	rightTargetOuter_->Draw();
	rightTargetInner_->Draw();

	ParticleManager::GetInstance()->Draw();

	result_->Draw();

	// --------------------------------------------------------------------//

	if (fade_) { fade_->Draw(); }
}

void BossTestScene::Finalize() {}

void BossTestScene::CheckCollisions() {

	// -------------------- 弾とボス部位の当たり判定 -------------------- //
	{
		auto& bullets = player_->GetBullets();
		auto body = boss_->GetBody();
		auto left = boss_->GetLeftArm();
		auto right = boss_->GetRightArm();

		for (auto it = bullets.begin(); it != bullets.end();) {
			bool hit = false;

			// 判定対象（パーツごと）
			std::vector<Object3d*> parts = { left, right, body };
			for (auto* part : parts) {
				// 
				const Vector3 partPos = part->GetWorldPosition();
				const float   d = MyMath::CalculateDistance((*it)->GetTranslate(), partPos);
				const float   r = (*it)->GetRadius() + part->GetRadius();

				if (d < r) {
					// パーティクル位置＝弾の位置
					const Vector3 hitPos = (*it)->GetTranslate();
					emitter_->SetTranslate(hitPos);
					emitter_->Update();

					// どの部位に当たったかで加算先を分ける
					if (part == left) {
						boss_->AddHitLeftArm();
						leftTargetShakeTime_ = targetShakeDuration_;   // 左ターゲット揺らす
					} else if (part == right) {
						boss_->AddHitRightArm();
						rightTargetShakeTime_ = targetShakeDuration_;  // 右ターゲット揺らす
					} else {
						boss_->Damage(1);
					}


					it = bullets.erase(it);
					hit = true;
					break;
				}
			}

			if (!hit) { ++it; }
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

			if (!koActive_ && player_->GetHP() <= 0) {
				KnockoutCameraController::Params p;
				p.fallSide = +1;                    // 左へなら -1
				ko_.Start(camera_.get(), /*groundY*/ 0.0f, p);
				koActive_ = true;
				isCameraFollowPlayer_ = false;
			}

			// カメラを揺らす
			if (camera_) {
				camera_->StartShake(CameraShakeType::Medium);
			}
			break;
		}
	}

	// -------------------- プレイヤー vs メテオ -------------------- //
	{
		Vector3 ppos = player_->GetTranslate();
		float   pr = player_->GetRadius();
		for (auto& m : meteors_) {
			if (!m->IsAlive()) continue;
			Vector3 mpos = m->GetObject()->GetWorldPosition();
			float   mr = m->GetRadius();

			if (MyMath::CalculateDistance(ppos, mpos) < (pr + mr)) {

				if (!player_->GetInvincible()) {
					player_->Damage(1);
					player_->SetInvincible(true);

					// ★ HP0ならノックアウト開始（腕の処理と同じ）
					if (!koActive_ && player_->GetHP() <= 0) {
						KnockoutCameraController::Params p;
						p.fallSide = +1; // 好きな方向でOK
						ko_.Start(camera_.get(), /*groundY*/ 0.0f, p);
						koActive_ = true;
						isCameraFollowPlayer_ = false;
					}
				}

				if (camera_) {
					camera_->StartShake(CameraShakeType::Medium);
				}

				m->Explode();
				break;
			}
		}
	}

	// -------------------- プレイヤー弾 vs 剣 -------------------- //

	if (sword_ && sword_->IsAlive()) {
		auto& bullets = player_->GetBullets();
		for (auto it = bullets.begin(); it != bullets.end();) {
			const Vector3 bpos = (*it)->GetTranslate();
			const float   br = (*it)->GetRadius();
			const Vector3 spos = sword_->GetPos();
			const float   sr = sword_->GetRadius();

			if (MyMath::CalculateDistance(bpos, spos) < (br + sr)) {
				// 命中演出（パーティクル/SE/小シェイク）
				emitter_->SetTranslate(bpos); emitter_->Update();
				camera_->StartShake(CameraShakeType::Small);

				bool destroyed = sword_->OnHitByBullet();
				it = bullets.erase(it);
				// 破壊できたらパリィ成功
				if (destroyed) {
					camera_->StartShake(CameraShakeType::Medium);

					// 反射方向：カメラの forward 方向の逆（= 画面奥＝ボス側）
					Vector3 camRot = camera_->GetRotate();
					float cp = std::cos(camRot.x), sp = std::sin(camRot.x);
					float cy = std::cos(camRot.y), sy = std::sin(camRot.y);
					Vector3 forward = { sy * cp, -sp, cy * cp };

					sword_->ReflectTo(forward, /*speed*/1.6f);  // 画面奥へ飛ばす
				} else {
					++it;
				}
			}
		}
	}

	// -------------------- プレイヤー vs 剣 -------------------- //

	if (sword_ && sword_->IsAlive() && !sword_->IsBroken()) {
		Vector3 ppos = player_->GetTranslate();
		float   pr = player_->GetRadius();
		if (MyMath::CalculateDistance(ppos, sword_->GetPos()) < (pr + sword_->GetRadius())) {
			if (!player_->GetInvincible()) {
				player_->Damage(1);
				player_->SetInvincible(true);
				camera_->StartShake(CameraShakeType::Large);
			}
			// ヒット後は剣を消す
			// （斬り抜け演出したいなら alive 継続でもOK）
			// ここでは消す：
			sword_->OnHitByBullet(); // 強制破壊扱い
		}
	}

	// -------------------- プレイヤー弾 vs メテオ -------------------- //
	{
		auto& bullets = player_->GetBullets();
		for (auto it = bullets.begin(); it != bullets.end();)
		{
			bool hitMeteor = false;

			for (auto& m : meteors_) {
				if (!m->IsAlive()) { continue; }

				const Vector3 bpos = (*it)->GetTranslate();
				const float   br = (*it)->GetRadius();

				const Vector3 mpos = m->GetObject()->GetWorldPosition();
				const float   mr = m->GetRadius();

				const float dist = MyMath::CalculateDistance(bpos, mpos);
				if (dist < (br + mr)) {

					// 命中エフェクト（位置は弾の位置でOK）
					const Vector3 hitPos = bpos;
					if (emitter_) {
						emitter_->SetTranslate(hitPos);
						emitter_->Update();
					}
					// メテオを爆発させて消す
					m->Explode();
					// 弾も削除
					it = bullets.erase(it);
					hitMeteor = true;
					break;
				}
			}
			if (!hitMeteor) {
				++it;
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

	// ★ 追加：銃の先端（今はモデルの原点）をプレイヤーに渡す
	if (player_) {
		// もし本当に「銃の先」にしたければ forward に少し足す
		Vector3 muzzle = gun_->GetWorldPosition() + forward * 1.0f; // 1.0f は好みで調整
		player_->SetGunMuzzlePos(muzzle);
	}
}

void BossTestScene::UpdateMeteorControl(float dt)
{
	// デバッグ用トグル（Mキーで開始／強制終了）
	if (System::TriggerKey(DIK_M) && meteorController_) {
		if (!meteorController_->IsActive()) {
			meteorController_->Start();
		} else {
			meteorController_->ForceEnd();
		}
	}

	// ボスからのリクエストでメテオ開始
	if (meteorController_
		&& !meteorController_->IsActive()
		&& boss_
		&& boss_->ConsumeMeteorRequest()) {
		meteorController_->Start();
	}

	// メテオ中はカメラ＆メテオはコントローラに任せる
	if (meteorController_ && meteorController_->IsActive()) {
		meteorController_->Update(dt);
	}

	// ↓ メテオ中じゃないときだけ、従来どおりプレイヤー追従カメラ
	Vector3 playerPos = player_->GetTransform().translate;
	Vector3 playerRot = player_->GetTransform().rotate;

	if ((!meteorController_ || !meteorController_->IsActive()) && isCameraFollowPlayer_) {
		camera_->SetTranslate(playerPos);
		camera_->SetRotate(playerRot);
	}
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
	// WaitMeteor 中などはどれにも当てはまらない

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

	// ---- 左手ターゲット表示条件 ----
	bool showLeft =
		// 左片手攻撃中 か
		(isLeftAttack && leftHits < maxHits) ||
		// 両手攻撃中で、左が規定未満
		(isBothAttack && leftHits < maxHits);

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
	}

	// ---- 右手ターゲット表示条件 ----
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
	}
}