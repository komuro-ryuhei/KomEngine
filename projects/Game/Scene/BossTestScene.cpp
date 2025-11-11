#include "BossTestScene.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include "Engine/Base/System/System.h"

#include <memory>
#include <vector>

void BossTestScene::Init() {

	const std::string &uvTexture = "./Resources/images/uvChecker.png";
	const std::string &circle = "./Resources/images/circle.png";
	const std::string &circle2 = "./Resources/images/circle2.png";
	const std::string &monsterBallTexture = "./Resources/images/monsterBall.png";
	const std::string &ring = "./Resources/images/gradationLine.png";
	const std::string &moonLight = "./Resources/images/moonLight.png";

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

	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");
	ModelManager::GetInstance()->LoadModel("cube.obj");
	ModelManager::GetInstance()->LoadModel("Player.obj");
	ModelManager::GetInstance()->LoadModel("Enemy.obj");
	ModelManager::GetInstance()->LoadModel("ground.obj");
	ModelManager::GetInstance()->LoadModel("hand.obj");
	ModelManager::GetInstance()->LoadModel("BossEnemy.obj");

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

	// Boss
	boss_ = std::make_unique<BossEnemy>();
	boss_->Init(camera_.get());
	boss_->SetTranslate({ 0.0f, 0.0f, 20.0f });
	boss_->SetPlayer(player_.get());

	auto makeTarget = [](std::unique_ptr<Sprite> &outer, std::unique_ptr<Sprite> &inner)
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

	// ボスの剣
	sword_ = std::make_unique<BossSword>();
	sword_->Init(camera_.get());

	// パーティクル
	auto *pm = ParticleManager::GetInstance();
	pm->Init(camera_.get(), BlendType::BLEND_ADD);

	pm->CreateParticleGeoup("hit", circle2, "a");
	pm->CreateParticleGeoup("explosion", monsterBallTexture, "a");
	pm->CreateParticleGeoup("ring", ring, "ring");
	pm->CreateParticleGeoup("cylinder", ring, "cylinder");
	pm->CreateParticleGeoup("moonLight", moonLight, "moonLight");
	pm->CreateParticleGeoup("ribbon", moonLight, "ribbon");

	// グループが既にあれば作らない
	if (!pm->Exists("hit")) {
		pm->CreateParticleGeoup("hit", "./Resources/images/circle2.png", "hit");
	}

	emitter_ = std::make_unique<ParticleEmitter>();
	emitter_->Init("hit", { 0.0f, 0.0f, 10.0f }, 10);
}

void BossTestScene::Update() {

	const float dt = 1.0f / 60.0f;

	// デバッグ用トグル（Mキーで開始）
	if (System::TriggerKey(DIK_M)) {
		if (meteorPhase_ == MeteorPhase::kIdle) StartMeteorMode();
		else if (meteorPhase_ != MeteorPhase::kIdle) EndMeteorMode();
	}

	// ボスからのリクエストでメテオ開始
	if (meteorPhase_ == MeteorPhase::kIdle && boss_ && boss_->ConsumeMeteorRequest()) {
		StartMeteorMode();
	}

	// メテオ中は自動追従を一時停止して、こちらの制御に任せる
	if (meteorPhase_ == MeteorPhase::kIdle && isCameraFollowPlayer_) {
		camera_->SetTranslate(player_->GetTransform().translate);
		camera_->SetRotate(player_->GetTransform().rotate);
	} else if (meteorPhase_ != MeteorPhase::kIdle) {
		UpdateMeteorMode(dt);  // カメラ補間とメテオ処理
	}

	// 剣攻撃トリガー
	if (System::TriggerKey(DIK_J) && !swordAttack_) {
		swordAttack_ = true;
		swordPhaseT_ = 0.f;
	}

	Vector3 playerPos = player_->GetTransform().translate;
	Vector3 playerRot = player_->GetTransform().rotate;

	// フラグがtrueだと追従（※メテオ中はしない）
	if (meteorPhase_ == MeteorPhase::kIdle && isCameraFollowPlayer_) {
		camera_->SetTranslate(playerPos);
		camera_->SetRotate(playerRot);
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

			// フェードアウト開始
			if (fade_ && phase_ == Phase::kMain) {
				fade_->Start(Fade::Status::FadeOut, 0.6f); // 0.6秒の黒フェード
				phase_ = Phase::kFadeOut;
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

	if (!koActive_ && meteorPhase_ == MeteorPhase::kIdle && !swordCamActive_) {

		// 伸ばし始めた瞬間に一回だけ開始
		if (!armCamActive_ && boss_->IsExtending()) {

			armCamActive_ = true;
			armCamT_ = 0.0f;

			// 元の向き保存
			armCamSavedRot_ = camera_->GetRotate();

			// 腕方向の目標回転を計算
			Vector3 camPos = camera_->GetTranaslate();
			Vector3 armPos = boss_->GetCurrentArmWorldPos();
			Vector3 to = MyMath::Normalize(armPos - camPos);

			float pitch = -std::asin(to.y);
			float yaw = std::atan2(to.x, to.z);

			Vector3 fullLook = { pitch, yaw, 0.0f };

			// ガッツリ向けると違和感出るので、少しだけ腕方向を混ぜる
			const float lookWeight = 0.35f; // 0.2〜0.5くらいで好み調整
			armCamTargetRot_ = MyMath::Lerp(armCamSavedRot_, fullLook, lookWeight);
		}

		if (armCamActive_) {
			if (boss_->IsExtending()) {
				// 腕が伸びている間：腕方向へ「じわっ」と向ける
				armCamT_ += dt / armCamIntroTime_;
				float t = MyMath::Clamp01(armCamT_);
				camera_->SetRotate(MyMath::Lerp(armCamSavedRot_, armCamTargetRot_, t));
			} else {
				// 腕が戻り始めたら：元の向きへ戻す
				armCamT_ += dt / armCamOutroTime_;
				float t = MyMath::Clamp01(armCamT_);
				camera_->SetRotate(MyMath::Lerp(armCamTargetRot_, armCamSavedRot_, t));

				if (t >= 1.0f) {
					armCamActive_ = false;
					armCamT_ = 0.0f;
				}
			}
		}
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

	// プレイヤー
	player_->Update();
	// ボス
	boss_->Update();
	// ボスのメテオ攻撃用
	for (auto &m : meteors_) m->Update();
	// ボスの剣
	if (sword_) sword_->Update();

	CheckCollisions();

	UpdateArmTargetMarker();

	leftTargetOuter_->Update();
	leftTargetInner_->Update();
	rightTargetOuter_->Update();
	rightTargetInner_->Update();


	// パーティクルの更新処理
	ParticleManager::GetInstance()->Update();

	{
		constexpr int LOW_HP_THRESHOLD = 0; // HP2以下
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
		// 遷移トリガー
		if (System::TriggerKey(DIK_RETURN) || boss_->GetHP() <= 0) {
			fade_->Start(Fade::Status::FadeOut, 0.6f);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			Fade::SetDefaultOpenModeSlash(false);
			sceneManager_->ChangeScene("TITLE");
		}
		break;
	}

#ifdef USE_IMGUI

	ImGui::Begin("BossTestScene");

	glassObject_->ImGuiDebug();
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
	glassObject_->Draw();

	// -------------------- ゲームオブジェクトシーンの描画 -------------------- //

	// Bossの描画
	boss_->Draw();
	// Bossのメテオ描画
	for (auto &m : meteors_) m->Draw();
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

	// --------------------------------------------------------------------//

	if (fade_) { fade_->Draw(); }
}

void BossTestScene::Finalize() {}

void BossTestScene::CheckCollisions() {

	// -------------------- 弾とボス部位の当たり判定 -------------------- //
	{
		auto &bullets = player_->GetBullets();
		auto body = boss_->GetBody();
		auto left = boss_->GetLeftArm();
		auto right = boss_->GetRightArm();

		for (auto it = bullets.begin(); it != bullets.end();) {
			bool hit = false;

			// 判定対象（パーツごと）
			std::vector<Object3d *> parts = { body, left, right };
			for (auto *part : parts) {
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
		Object3d *object;
		std::string name;
	};

	std::vector<ArmData> arms = {
		{ boss_->GetLeftArm(),  "LeftArm" },
		{ boss_->GetRightArm(), "RightArm" }
	};

	for (const auto &arm : arms) {
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
		for (auto &m : meteors_) {
			if (!m->IsAlive()) continue;
			Vector3 mpos = m->GetObject()->GetWorldPosition();
			float   mr = m->GetRadius();

			if (MyMath::CalculateDistance(ppos, mpos) < (pr + mr)) {
				if (!player_->GetInvincible()) {
					player_->Damage(1);
					player_->SetInvincible(true);
				}
				if (camera_) camera_->StartShake(CameraShakeType::Medium);
				m->Explode();
				break;
			}
		}
	}

	// -------------------- プレイヤー弾 vs メテオ -------------------- //
	{
		auto &bullets = player_->GetBullets();
		for (auto it = bullets.begin(); it != bullets.end(); ) {
			bool removed = false;

			for (auto &m : meteors_) {
				if (!m->IsAlive()) continue;

				// メテオの中心位置と半径
				const Vector3 mpos = m->GetObject()->GetWorldPosition(); // もしくは m->GetTransform().translate
				const float   mr = m->GetRadius();

				// 弾の中心と半径
				const Vector3 bpos = (*it)->GetTranslate();
				const float   br = (*it)->GetRadius();

				if (MyMath::CalculateDistance(bpos, mpos) < (br + mr)) {
					// ヒット演出
					emitter_->SetTranslate(bpos);
					emitter_->Update();
					if (camera_) camera_->StartShake(CameraShakeType::Small);

					// メテオ破壊 & 弾削除
					m->Explode();
					it = bullets.erase(it);
					removed = true;
					break;
				}
			}
			if (!removed) ++it;
		}
	}

	// -------------------- プレイヤー弾 vs 剣 -------------------- //

	if (sword_ && sword_->IsAlive()) {
		auto &bullets = player_->GetBullets();
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
	}
}

void BossTestScene::StartMeteorMode() {
	meteorPhase_ = MeteorPhase::kIntro;
	meteorModeTimer_ = 0.0f;
	spawnTimer_ = 0.0f;

	// 現在のカメラ状態を保存
	savedCamPos_ = camera_->GetTranaslate();
	savedCamRot_ = camera_->GetRotate();
}

void BossTestScene::UpdateMeteorMode(float dt) {
	Vector3 playerPos = player_->GetTransform().translate;

	switch (meteorPhase_) {
	case MeteorPhase::kIntro: {
		meteorModeTimer_ += dt;
		camLerp_ = std::min(1.0f, meteorModeTimer_ / camIntroTime_);

		// 目標カメラ：プレイヤー位置 + 少し上、ピッチだけ上向きに
		Vector3 targetPos = playerPos + targetCamPosOffset_;
		Vector3 targetRot = savedCamRot_;
		targetRot.x = targetPitchUp_;

		// 補間
		camera_->SetTranslate(MyMath::Lerp(savedCamPos_, targetPos, camLerp_));
		camera_->SetRotate(MyMath::Lerp(savedCamRot_, targetRot, camLerp_));

		if (camLerp_ >= 1.0f) {
			meteorPhase_ = MeteorPhase::kShower;
			meteorModeTimer_ = 0.0f;
		}
		break;
	}

	case MeteorPhase::kShower: {

		meteorModeTimer_ += dt;
		spawnTimer_ += dt;

		// カメラはプレイヤー位置を追いながら“上向き固定”
		camera_->SetTranslate(playerPos + targetCamPosOffset_);
		Vector3 rot = camera_->GetRotate();
		rot.x = targetPitchUp_;
		camera_->SetRotate(rot);

		// スポーン
		if (spawnTimer_ >= spawnInterval_) {
			spawnTimer_ = 0.0f;

			// --- カメラ姿勢 ---
			Vector3 camPos = camera_->GetTranaslate();
			Vector3 camRot = camera_->GetRotate(); // rot.x = pitch, rot.y = yaw

			// --- カメラ基底ベクトル ---
			float cp = std::cos(camRot.x), sp = std::sin(camRot.x);
			float cy = std::cos(camRot.y), sy = std::sin(camRot.y);

			// 前方（rot=0 で +Z）
			Vector3 forward = { sy * cp, -sp, cy * cp };
			Vector3 right = { cy, 0.0f, -sy };
			Vector3 up = { 0.0f, 1.0f, 0.0f };

			// ---- 発生位置：前方かなり遠く（地平線付近）----
			float dist = MyMath::Rand(110.0f, 160.0f); // ★遠く
			float spreadX = MyMath::Rand(-8.0f, 8.0f);    // 左右
			float spreadUp = MyMath::Rand(8.0f, 18.0f);   // 少し上

			Vector3 start = camPos + forward * dist + right * spreadX + up * spreadUp;

			// ---- ターゲット：カメラの “すぐ手前”（= forward の少し内側）----
			// これで進行方向はほぼ -forward、画面奥→手前へ突っ込んでくる
			Vector3 target = camPos + forward * 6.0f + up * (-2.0f);

			// 距離に応じて速度を上げる（遠いほど速い）
			float speed = 0.25f + 0.012f * dist;      // dist=120 → speed=1.69 くらい

			// 空きスロットに生成
			for (auto &m : meteors_) {
				if (!m->IsAlive()) {
					m->SetScale({ 1.5f, 1.5f, 1.5f });
					m->SetGravity(0.0f);
					m->Spawn(start, target, speed);
					break;
				}
			}
		}

		// 終了判定
		if (meteorModeTimer_ >= meteorModeDuration_) {
			meteorPhase_ = MeteorPhase::kOutro;
			meteorModeTimer_ = 0.0f;
		}
		break;
	}

	case MeteorPhase::kOutro: {
		meteorModeTimer_ += dt;
		camLerp_ = std::min(1.0f, meteorModeTimer_ / camOutroTime_);

		// 目標は保存していた通常カメラ
		Vector3 curPos = camera_->GetTranaslate();
		Vector3 curRot = camera_->GetRotate();

		camera_->SetTranslate(MyMath::Lerp(curPos, savedCamPos_, camLerp_));
		camera_->SetRotate(MyMath::Lerp(curRot, savedCamRot_, camLerp_));

		if (camLerp_ >= 1.0f) {
			EndMeteorMode();
		}
		break;
	}

	case MeteorPhase::kIdle: default: break;
	}
}

void BossTestScene::EndMeteorMode() {

	meteorPhase_ = MeteorPhase::kIdle;
	camLerp_ = 0.0f;

	for (auto &m : meteors_) {
		if (m->IsAlive()) m->Explode();
	}

	camera_->SetTranslate(savedCamPos_);
	camera_->SetRotate(savedCamRot_);

	// ★ ボスに「メテオ終わったよ」と伝える
	if (boss_) {
		boss_->OnMeteorFinished();
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

	auto projectToScreen = [&](const Vector3 &worldPos, Vector2 &outScreen) -> bool {

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

	auto hide = [](std::unique_ptr<Sprite> &o, std::unique_ptr<Sprite> &i) {
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