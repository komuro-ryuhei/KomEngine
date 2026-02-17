#include "Player.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include "Engine/Base/System/System.h"
#include "Engine/Base/Particle/ParticleManager.h"

#include <iostream>
#include <algorithm>

float Player::GetRadius() const { return radius_; }

Transform Player::GetTransform() const { return transform_; }

Vector3 Player::GetTranslate() const { return transform_.translate; }

std::vector<std::unique_ptr<PlayerBullet>>& Player::GetBullets() { return bulletObjects_; }

void Player::SetInvincible(bool flag) {

	isInvincible_ = flag;
	if (flag) {
		invincibleTimer_ = 1.0f;
	}
}

void Player::SetRotate(Vector3& rotate) { transform_.rotate = rotate; }

int Player::GetHP() const { return hp_; }

bool Player::GetInvincible() const { return isInvincible_; }

void Player::Damage(int amount) {

	hp_ -= amount;

	if (camera_) {
		camera_->StartShake(CameraShakeType::Small);
	}
}

bool Player::IsLowHP(int hp) const { return hp_ <= hp; }

bool Player::IsInvincible() const { return isInvincible_; }

Player::~Player() {

	// 
	bulletObjects_.clear();
}

void Player::Init(Camera* camera) {

	camera_ = camera;

	// 自機、弾の生成
	object3d_ = std::make_unique<Object3d>();
	object3d_->Init(BlendType::BLEND_NONE);

	object3d_->SetModel("sphere.obj");
	object3d_->SetDefaultCamera(camera_);

	object3d_->SetScale({ 0.5f, 0.5f, 0.5f });

	// レティクルのスプライトを生成
	reticleSprite_ = std::make_unique<Sprite>();
	reticleSprite_->Init("./Resources/images/reticle.png", BlendType::BLEND_ALPHA);
	reticleSprite_->SetSize({ 100.0f, 100.0f });
	// 初期位置を画面中央へ
	reticleSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	reticleSprite_->SetPosition({ 1280.0f * 0.5f, 720.0f * 0.5f });

	// 手元Gun
	gun_ = std::make_unique<Object3d>();
	gun_->Init(BlendType::BLEND_NONE);
	gun_->SetModel("cube.obj");
	gun_->SetDefaultCamera(camera_);
	gun_->SetScale(gunScale_);
	gun_->SetRotate(gunRotate_);
	gun_->SetTranslate(gunTranslate_);

	// マズルフラッシュ用エミッターの初期化
	muzzleEmitter_ = std::make_unique<ParticleEmitter>();
	// 名前 "muzzle"、初期座標はとりあえず原点、1回に出す粒の数はお好みで（ここでは12）
	muzzleEmitter_->Init("muzzle", { 0.0f, 0.0f, 0.0f }, 12);

	// チャージ用エミッター
	chargeCoreEmitter_ = std::make_unique<ParticleEmitter>();
	chargeCoreEmitter_->Init("charge_core", { 0.0f, 0.0f, 0.0f }, 6);   // 常時キラキラ
	chargePulseEmitter_ = std::make_unique<ParticleEmitter>();
	chargePulseEmitter_->Init("charge_pulse", { 0.0f, 0.0f, 0.0f }, 1); // たまにリング

	// オーバーヒートゲージのスプライト
	heatGaugeBg_ = std::make_unique<Sprite>();
	heatGaugeBg_->Init("./Resources/images/blackBG.png", BlendType::BLEND_ALPHA);
	heatGaugeBg_->SetSize({ heatGaugeMaxWidth_, heatGaugeHeight_ });
	heatGaugeBg_->SetAnchorPoint({ 0.0f, 1.0f });
	heatGaugeBg_->SetPosition(heatGaugePos_);

	heatGaugeFill_ = std::make_unique<Sprite>();
	heatGaugeFill_->Init("./Resources/images/gauge.png", BlendType::BLEND_ALPHA);
	heatGaugeFill_->SetSize({ heatGaugeMaxWidth_, heatGaugeHeight_ });
	heatGaugeFill_->SetAnchorPoint({ 0.0f, 1.0f });
	heatGaugeFill_->SetPosition(heatGaugePos_);
}

void Player::Update() {

	const float dt = System::GetDeltaTime();

	firedThisFrame_ = false;

	// 連射タイマーを減算
	autofireTimer_ = std::max(0.0f, autofireTimer_ - dt);

	if (controlEnabled_) {
		Attack(dt);
	}

	ChargeEffect(dt);

	// ----------------------- オーバーヒート冷却処理 ----------------------- //
	{
		// 撃ったフレームは冷却しない
		if (!firedThisFrame_) {
			const float coolPerSec = isCharging_ ? heatCoolWhileCharge_ : heatCoolPerSec_;
			heat_ = std::max(0.0f, heat_ - coolPerSec * dt);
		}

		// 復帰判定
		if (isOverheated_ && heat_ <= heatRecover_) {
			isOverheated_ = false;
			canShoot_ = true;
		}
		// 念のための上限
		heat_ = std::clamp(heat_, 0.0f, heatMax_);
	}

	// 無敵タイマー処理
	if (isInvincible_) {
		invincibleTimer_ -= dt; // 毎フレーム減少
		if (invincibleTimer_ <= 0.0f) {
			isInvincible_ = false;
			invincibleTimer_ = 0.0f;
		}
	}

	// 弾更新と描画
	for (auto it = bulletObjects_.begin(); it != bulletObjects_.end(); ) {
		(*it)->Update();
		(*it)->ImGuiDebug();
		if (!(*it)->IsAlive()) {

			if (collisionManager_) {
				collisionManager_->Unregister(it->get());
			}

			it = bulletObjects_.erase(it);
		} else {
			++it;
		}
	}

	UpdateGun();

	if (!controlEnabled_) {
		// Intro中は弾も更新しない（発射もされない）
		object3d_->Update();
		return;
	}

	object3d_->SetTranslate(transform_.translate);
	object3d_->SetRotate(transform_.rotate);

	UpdateReticleSprite();

	UpdateHeatGauge();
}

void Player::Draw() {

	// 
	// object3d_->Draw();

	for (auto& bullet : bulletObjects_) {
		bullet->Draw();
	}

	/*if (gun_) {
		gun_->Draw();
	}*/

	if (heatGaugeBg_) { heatGaugeBg_->Draw(); }
	if (heatGaugeFill_) { heatGaugeFill_->Draw(); }

	reticleSprite_->Draw();

	//// ロックオンスプライトの描画
	// if (lockedTarget_ && lockedTarget_->GetIsAlive()) {
	//	lockOnSprite_->Draw();
	// }
}

void Player::ImGuiDebug() {

#ifdef USE_IMGUI

	ImGui::Begin("Player");

	gun_->ImGuiDebug("gun");

	ImGui::SliderAngle("rotateX", &transform_.rotate.x, 0.1f);
	ImGui::SliderAngle("rotateY", &transform_.rotate.y, 0.1f);
	ImGui::SliderAngle("rotateZ", &transform_.rotate.z, 0.1f);
	ImGui::DragFloat3("translate", &transform_.translate.x, 0.1f);
	ImGui::DragInt("HP", &hp_);

	ImGui::Separator();
	ImGui::Text("Heat");
	ImGui::DragFloat("heat", &heat_, 0.1f, 0.0f, heatMax_);
	ImGui::DragFloat("heatMax", &heatMax_, 0.1f, 1.0f, 999.0f);
	ImGui::DragFloat("heatRecover", &heatRecover_, 0.1f, 0.0f, heatMax_);
	ImGui::DragFloat("coolPerSec", &heatCoolPerSec_, 0.1f, 0.0f, 999.0f);
	ImGui::DragFloat("coolWhileCharge", &heatCoolWhileCharge_, 0.1f, 0.0f, 999.0f);
	ImGui::DragFloat("costNormal", &heatCostNormal_, 0.1f, 0.0f, 999.0f);
	ImGui::DragFloat("costAutofire", &heatCostAutofire_, 0.1f, 0.0f, 999.0f);
	ImGui::DragFloat("costCharged", &heatCostCharged_, 0.1f, 0.0f, 999.0f);
	ImGui::Text("Overheated: %s", isOverheated_ ? "YES" : "NO");

	ImGui::Separator();
	ImGui::Text("Charge");
	ImGui::Text("Charging: %s", isCharging_ ? "YES" : "NO");
	ImGui::DragFloat("chargeTimer", &chargeTimer_, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("chargeMin", &chargeMinTime_, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("chargeFull", &chargeFullTime_, 0.01f, 0.0f, 10.0f);

	ImGui::End();

#endif // _DEBUG
}

void Player::Attack(float dt) {

	if (!canShoot_) { return; }

	auto* input = System::GetInput();

	const bool mouse0Down = input->PushMouse(1); //左左クリック想定
	const bool mouse1Down = input->PushMouse(0); // 右クリック長押し連射

	// -------- チャージ（Mouse0：押し→離し）--------
	if (mouse0Down && !prevMouse0Down_) {
		isCharging_ = true;
		chargeTimer_ = 0.0f;
	}

	if (mouse0Down && isCharging_) {
		chargeTimer_ += dt;
	}

	// 離した瞬間に発射
	if (!mouse0Down && prevMouse0Down_ && isCharging_) {

		// チャージ量を 0〜1 に正規化
		float t = chargeTimer_ / chargeFullTime_;
		t = std::clamp(t, 0.0f, 1.0f);

		// 1〜5 に増やす（最大ダメージ5）
		int damage = 1 + static_cast<int>(t * 4.0f);

		SpawnBullet(damage);

		isCharging_ = false;
		chargeTimer_ = 0.0f;
	}

	// -------- 連射（Mouse1長押し）--------
	if (mouse1Down && autofireTimer_ <= 0.0f) {

		const float cost = heatCostAutofire_;

		if (heat_ + cost >= heatMax_) {
			heat_ = heatMax_;
			isOverheated_ = true;
			canShoot_ = false;
		} else {
			heat_ += cost;
			SpawnBullet(1);
			autofireTimer_ = autofireInterval_;
		}
	}

	prevMouse0Down_ = mouse0Down;
}

void Player::RailMove() { transform_.translate.z += velocity_; }

void Player::RotateY90() {
	transform_.rotate.y += 90.0f;

	// 360度を超えないように正規化
	if (transform_.rotate.y >= 360.0f) {
		transform_.rotate.y -= 360.0f;
	}

	// Object3Dにも反映
	if (object3d_) {
		object3d_->SetRotate(transform_.rotate);
	}
}

void Player::SpawnBullet(int damage) {

	damage = std::max(1, damage);

	// マズルフラッシュ（位置は元のまま）
	Vector3 muzzlePos = transform_.translate;

	/*if (muzzleEmitter_) {
		muzzleEmitter_->SetTranslate(muzzlePos);
		muzzleEmitter_->Update();
	}*/

	// ----------------------------
	// 弾オブジェクト生成
	// ----------------------------
	Object3d* bulletObject = new Object3d();
	bulletObject->Init(BlendType::BLEND_NONE);
	bulletObject->SetModel("sphere.obj");
	bulletObject->SetDefaultCamera(camera_);

	auto newBullet = std::make_unique<PlayerBullet>();
	newBullet->Init(camera_, bulletObject);

	// ダメージを弾に設定
	newBullet->SetDamage(damage);

	// ----------------------------
	// damageから強さ(power)を作る
	// ----------------------------
	const int maxDamage = 5; // 好きに調整OK

	float t = 0.0f;
	if (maxDamage > 1) {
		t = float(damage - 1) / float(maxDamage - 1);
	}
	t = std::clamp(t, 0.0f, 1.0f);

	float power = 1.0f + t * 2.0f; // 1.0〜3.0

	// 見た目
	float visualScale = 0.1f * power;
	newBullet->SetScale({ visualScale, visualScale, visualScale });

	// 当たり判定
	float radius = 0.08f * power;
	newBullet->SetRadius(radius);

	// 速度
	float speed = 0.01f * (1.0f + 0.25f * (power - 1.0f));
	newBullet->SetSpeed(speed);

	// ----------------------------
	// 発射方向（レティクル）
	// ----------------------------
	Vector3 direction;

	if (reticleSprite_) {

		Matrix4x4 viewMatrix = camera_->GetViewMatrix();
		Matrix4x4 projMatrix = camera_->GetProjectionMatrix();
		Matrix4x4 vpMatrix = MyMath::Multiply(viewMatrix, projMatrix);
		Matrix4x4 invVPMatrix = MyMath::Inverse4x4(vpMatrix);

		Matrix4x4 viewportMatrix =
			MyMath::MakeViewportMatrix(0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f);
		Matrix4x4 invViewportMatrix = MyMath::Inverse4x4(viewportMatrix);

		Vector2 spritePos = reticleSprite_->GetCenterPosition();
		Vector3 screenNear = { spritePos.x, spritePos.y, 0.0f };
		Vector3 screenFar = { spritePos.x, spritePos.y, 1.0f };

		Vector3 ndcNear = MyMath::Transform(screenNear, invViewportMatrix);
		Vector3 ndcFar = MyMath::Transform(screenFar, invViewportMatrix);

		Vector3 worldNear = MyMath::Transform(ndcNear, invVPMatrix);
		Vector3 worldFar = MyMath::Transform(ndcFar, invVPMatrix);

		direction = worldFar - worldNear;

	} else {
		direction = { 0.0f, 0.0f, 1.0f };
	}

	MyMath::Normalize(direction);
	newBullet->SetDirection(direction);

	// 出現位置は元のプレイヤー中心
	newBullet->SetTranlate(transform_.translate);

	if (collisionManager_) {
		collisionManager_->Register(newBullet.get());
	}

	bulletObjects_.emplace_back(std::move(newBullet));

	firedThisFrame_ = true;
}

void Player::UpdateGun() {

	if (!gun_ || !camera_) { return; }

	// viewの逆行列からカメラ座標系（位置/前/右/上）を取得
	Matrix4x4 view = camera_->GetViewMatrix();
	Matrix4x4 invView = MyMath::Inverse4x4(view);

	// カメラ原点をワールドへ
	Vector3 camPos = MyMath::Transform({ 0.0f, 0.0f, 0.0f }, invView);

	// カメラ基底（ワールド空間）
	Vector3 camFwd = MyMath::Transform({ 0.0f, 0.0f, 1.0f }, invView) - camPos;
	Vector3 camRight = MyMath::Transform({ 1.0f, 0.0f, 0.0f }, invView) - camPos;
	Vector3 camUp = MyMath::Transform({ 0.0f, 1.0f, 0.0f }, invView) - camPos;

	MyMath::Normalize(camFwd);
	MyMath::Normalize(camRight);
	MyMath::Normalize(camUp);

	// 手元位置：前方 + 右 + 下
	Vector3 gunPos = camPos + camFwd * gunDist_ + camRight * gunRight_ - camUp * gunDown_;

	// Gunに反映
	//gun_->SetTranslate(gunPos);
	//gun_->SetScale(gunScale_);

	// 回転：まずはカメラと同じ向き（見た目のズレがあればoffsetで調整）
	// Cameraに GetRotate() があるならそれを使うのが簡単だが、ここでは view 由来で済ませたいので
	// 「とりあえず回転は固定」でも成立する（Cubeなので）
	// もし SetRotate が必要なら、camera_->GetRotate() が存在する前提で↓を有効化して調整してください。
	//
	// Vector3 r = camera_->GetRotate();
	// r.x += gunRotOffset_.x;
	// r.y += gunRotOffset_.y;
	// r.z += gunRotOffset_.z;
	// gun_->SetRotate(r);

	gun_->Update();

	// 銃口位置も更新して、弾/マズル/チャージ演出の起点にする
	// Cubeの前方（+fwd方向）に少し出す
	hasGunMuzzlePos_ = true;
	gunMuzzlePos_ = gunPos + camFwd * 0.8f + camRight * 0.05f - camUp * 0.02f;
}

void Player::UpdateHeatGauge() {

	if (!heatGaugeBg_ || !heatGaugeFill_) { return; }

	float remain = 1.0f;
	if (heatMax_ > 0.0f) {
		remain = 1.0f - (heat_ / heatMax_);
	}
	remain = std::clamp(remain, 0.0f, 1.0f);

	const float w = heatGaugeMaxWidth_ * remain;
	heatGaugeFill_->SetSize({ w, heatGaugeHeight_ });

	heatGaugeBg_->SetPosition(heatGaugePos_);
	heatGaugeFill_->SetPosition(heatGaugePos_);

	heatGaugeBg_->Update();
	heatGaugeFill_->Update();
}

void Player::UpdateReticleSprite() {

	// マウスカーソルのスクリーン座標を取得
	POINT pt;
	GetCursorPos(&pt);

	// ゲームウィンドウのクライアント座標系に変換
	HWND hwnd = System::GetWinApp()->GetHwnd();
	ScreenToClient(hwnd, &pt);

	// レティクルへ反映
	Vector2 reticlePos = { (float)pt.x, (float)pt.y };

	// クランプ
	reticlePos.x = std::clamp(reticlePos.x, 0.0f, 1280.0f);
	reticlePos.y = std::clamp(reticlePos.y, 0.0f, 720.0f);

	reticleSprite_->SetPosition(reticlePos);
	reticleSprite_->Update();
}

void Player::ChargeEffect(float dt) {

	// コントロール無効中は出さない（Intro等）
	if (!controlEnabled_) {
		chargeFxCoreTimer_ = 0.0f;
		chargeFxPulseTimer_ = 0.0f;
		return;
	}
	if (!camera_ || !chargeCoreEmitter_ || !chargePulseEmitter_) { return; }

	// チャージしてないならタイマーだけリセット
	if (!isCharging_) {
		chargeFxCoreTimer_ = 0.0f;
		chargeFxPulseTimer_ = 0.0f;
		return;
	}

	// 0..1 のチャージ率
	float tCharge = chargeTimer_ / chargeFullTime_;
	tCharge = std::clamp(tCharge, 0.0f, 1.0f);

	// 青→黄→赤 のグラデ（2段補間）
	auto Lerp4 = [](const Vector4& a, const Vector4& b, float t) {
		return Vector4{
			a.x + (b.x - a.x) * t,
			a.y + (b.y - a.y) * t,
			a.z + (b.z - a.z) * t,
			a.w + (b.w - a.w) * t
		};
		};

	const Vector4 blue{ 0.20f, 0.55f, 1.00f, 1.0f };
	const Vector4 yellow{ 1.00f, 0.95f, 0.20f, 1.0f };
	const Vector4 red{ 1.00f, 0.20f, 0.20f, 1.0f };

	Vector4 coreColor;
	if (tCharge < 0.5f) {
		coreColor = Lerp4(blue, yellow, tCharge / 0.5f);
	} else {
		coreColor = Lerp4(yellow, red, (tCharge - 0.5f) / 0.5f);
	}

	// pulseは少し薄め/明るめに
	Vector4 pulseColor = coreColor;
	pulseColor.w = 0.85f;

	// ParticleManager に反映
	ParticleManager::GetInstance()->SetChargeEffectColor(coreColor, pulseColor);

	// ----------------------------
	// カメラ基準で「手元位置」を作る
	// viewの逆行列でカメラの座標系を取得
	// ----------------------------
	Matrix4x4 view = camera_->GetViewMatrix();
	Matrix4x4 invView = MyMath::Inverse4x4(view);

	Vector3 camPos = MyMath::Transform({ 0.0f, 0.0f, 0.0f }, invView);
	Vector3 camFwd = MyMath::Transform({ 0.0f, 0.0f, 1.0f }, invView) - camPos;
	Vector3 camRight = MyMath::Transform({ 1.0f, 0.0f, 0.0f }, invView) - camPos;
	Vector3 camUp = MyMath::Transform({ 0.0f, 1.0f, 0.0f }, invView) - camPos;

	MyMath::Normalize(camFwd);
	MyMath::Normalize(camRight);
	MyMath::Normalize(camUp);

	// 手元：カメラ前方 + 右下（邪魔になりにくい）
	const float handDist = 2.0f;    // 手元までの距離（見え方の要）
	const float rightOff = 0.35f;   // 右に寄せる
	const float downOff = 0.25f;   // 下に寄せる

	Vector3 handPos = camPos + camFwd * handDist + camRight * rightOff - camUp * downOff;

	// 「奥から吸い寄せ」用：奥の開始点
	Vector3 farPos = camPos + camFwd * 6.0f;

	// ----------------------------
	// 奥→手元へ吸い寄せる“細かい粒”
	// （邪魔にならないように少量）
	// ----------------------------
	chargeFxCoreTimer_ -= dt;
	if (chargeFxCoreTimer_ <= 0.0f) {

		// 1〜2点だけ撒く（画面の邪魔防止）
		const int spawnCount = 2;

		for (int i = 0; i < spawnCount; ++i) {
			float t = (float)(std::rand() % 100) / 100.0f; // 0..1
			Vector3 p = farPos + (handPos - farPos) * t;

			// 少しだけ散らす（吸い込まれる感じ）
			float jx = ((std::rand() % 100) / 100.0f - 0.5f) * 0.25f;
			float jy = ((std::rand() % 100) / 100.0f - 0.5f) * 0.25f;
			p = p + camRight * jx + camUp * jy;

			chargeCoreEmitter_->SetTranslate(p);
			chargeCoreEmitter_->Update();
		}

		// 手元にも1発（中心が光る）
		chargeCoreEmitter_->SetTranslate(handPos);
		chargeCoreEmitter_->Update();

		chargeFxCoreTimer_ = 0.04f; // 25fps相当
	}

	// ----------------------------
	// 手元で脈動リング（低頻度）
	// ----------------------------
	chargeFxPulseTimer_ -= dt;
	if (chargeFxPulseTimer_ <= 0.0f) {
		chargePulseEmitter_->SetTranslate(handPos);
		chargePulseEmitter_->Update();
		chargeFxPulseTimer_ = 0.20f;
	}
}

Vector3 Player::GetCollisionPosition() const
{
	return GetTranslate();  // 現状は中心＝Translate
}

float Player::GetCollisionRadius() const
{
	return GetRadius();     // Object3d の radius
}

CollisionLayer Player::GetCollisionLayer() const
{
	return CollisionLayer::Player;
}

void Player::OnCollision(ICollisionObject* other)
{
	switch (other->GetCollisionLayer()) {
		// 敵と当たった場合
	case CollisionLayer::Enemy:
		if (!GetInvincible()) {
			Damage(1);
			SetInvincible(true);
		}
		break;
		// 敵弾と当たった場合
	case CollisionLayer::EnemyBullet:
		if (!GetInvincible()) {
			// Damage(1);
			// SetInvincible(true);
		}
		break;

	default:
		break;
	}
}