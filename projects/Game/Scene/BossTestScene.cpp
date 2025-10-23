#include "BossTestScene.h"
#include "externals/imgui/imgui.h"
#include "Engine/Base/System/System.h"

#include <memory>
#include <vector>

void BossTestScene::Init() {

	// テクスチャ、モデルの読み込み
	TextureManager::GetInstance()->LoadTexture("./Resources/images/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/circle.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/circle2.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/test.dds");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/ground.png");

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

	// --- フェード初期化（画面サイズは 1280x720）---
	fade_ = std::make_unique<Fade>();
	fade_->Initialize(1280, 720);
	// fade_->Start(Fade::Status::FadeIn, 0.6f);
	fade_->StartSlashOpen(0.6f, 60.0f, true);
	phase_ = Phase::kFadeIn;

	// Player
	player_ = std::make_unique<Player>();
	player_->Init(camera_.get());

	// Boss
	boss_ = std::make_unique<BossEnemy>();
	boss_->Init(camera_.get());
	boss_->SetTranslate({ 0.0f, 0.0f, 20.0f });
	boss_->SetPlayer(player_.get());

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
	auto* pm = ParticleManager::GetInstance();
	pm->Init(camera_.get(), BlendType::BLEND_ADD);

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

	// プレイヤー
	player_->Update();
	// ボス
	boss_->Update();
	// ボスのメテオ攻撃用
	for (auto& m : meteors_) m->Update();
	// ボスの剣
	if (sword_) sword_->Update();

	CheckCollisions();

	// パーティクルの更新処理
	ParticleManager::GetInstance()->Update();

	if (swordAttack_) {

		// ① 右手を縮める（0.4秒）
		if (swordPhaseT_ < 0.4f) {
			swordPhaseT_ += dt;
			float t = std::min(1.f, swordPhaseT_ / 0.4f);
			// 右手のスケールを徐々に0へ（BossEnemy側にsetterが無いなら rightArmScale 直接）
			boss_->SetRightHandScale(MyMath::Lerp(Vector3{ 1,1,1 }, Vector3{ 0,0,0 }, t));
		}
		// ② 縮みきった直後：スイープ開始（カメラ演出なし版）
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

			// ★ここで即スイープ開始（カメラ演出は使わない）
			sword_->SetScale({ 1.6f,1.6f,1.6f });
			sword_->StartSweep(swordCenter_, swordRight_, swordForward_,
				swordHalfLen_, swordToward_, swordDuration_);

			// カメラ制御フラグは明示的にオフ
			swordCamActive_ = false;
			swordPendingSweep_ = false;
		}

		// ③ 剣が消えたら右手を戻して終了（0.3秒）
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
		if (System::TriggerKey(DIK_RETURN)) {
			fade_->Start(Fade::Status::FadeOut, 0.6f);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			sceneManager_->ChangeScene("TITLE");
		}
		break;
	}

#ifdef _DEBUG

	ImGui::Begin("BossTestScene");

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
	// glassObject_->Draw();

	// -------------------- ゲームオブジェクトシーンの描画 -------------------- //

	// Playerは一人称視点なので非描画
	player_->Draw();

	// Bossの描画
	boss_->Draw();
	// Bossのメテオ描画
	for (auto& m : meteors_) m->Draw();
	// Bossの剣描画
	if (sword_) sword_->Draw();

	// ParticleManager::GetInstance()->Draw();

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
			std::vector<Object3d*> parts = { body, left, right };
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
					} else if (part == right) {
						boss_->AddHitRightArm();
					} else {
						// 本体に命中したときの処理があればここに

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

			// HPが引数以下ならポストエフェクトを適応
			if (player_->IsLowHP(2)) {
				System::GetOffscreenRendering()->SetPostEffect("Vignetting");
			}

			// カメラを揺らす
			if (camera_) {
				camera_->StartShake(CameraShakeType::Medium);
			}
			break;
		}
	}

	// ---- プレイヤー vs メテオ ----
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
				}
				if (camera_) camera_->StartShake(CameraShakeType::Medium);
				m->Explode();
				break;
			}
		}
	}

	// -------------------- プレイヤー弾 vs メテオ -------------------- //
	{
		auto& bullets = player_->GetBullets();
		for (auto it = bullets.begin(); it != bullets.end(); ) {
			bool removed = false;

			for (auto& m : meteors_) {
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
			for (auto& m : meteors_) {
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

	// 生き残っているメテオは、まずは即消す
	for (auto& m : meteors_) {
		if (m->IsAlive()) m->Explode();
	}

	// カメラを元に
	camera_->SetTranslate(savedCamPos_);
	camera_->SetRotate(savedCamRot_);
}
