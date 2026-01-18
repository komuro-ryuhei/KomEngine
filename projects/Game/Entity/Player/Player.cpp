#include "Player.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include "Engine/Base/System/System.h"

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

	// マズルフラッシュ用エミッターの初期化
	muzzleEmitter_ = std::make_unique<ParticleEmitter>();
	// 名前 "muzzle"、初期座標はとりあえず原点、1回に出す粒の数はお好みで（ここでは12）
	muzzleEmitter_->Init("muzzle", { 0.0f, 0.0f, 0.0f }, 12);
}

void Player::Update() {

	const float dt = System::GetDeltaTime();

	// 連射タイマーを減算
	autofireTimer_ = std::max(0.0f, autofireTimer_ - dt);

	if (controlEnabled_) {
		Attack();
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

	if (!controlEnabled_) {
		// Intro中は弾も更新しない（発射もされない）
		object3d_->Update();
		return;
	}

	object3d_->SetTranslate(transform_.translate);
	object3d_->SetRotate(transform_.rotate);

	UpdateReticleSprite();
}

void Player::Draw() {

	// 
	// object3d_->Draw();

	for (auto& bullet : bulletObjects_) {
		bullet->Draw();
	}

	reticleSprite_->Draw();

	//// ロックオンスプライトの描画
	// if (lockedTarget_ && lockedTarget_->GetIsAlive()) {
	//	lockOnSprite_->Draw();
	// }
}

void Player::ImGuiDebug() {

#ifdef USE_IMGUI

	ImGui::Begin("Player");

	ImGui::SliderAngle("rotateX", &transform_.rotate.x, 0.1f);
	ImGui::SliderAngle("rotateY", &transform_.rotate.y, 0.1f);
	ImGui::SliderAngle("rotateZ", &transform_.rotate.z, 0.1f);
	ImGui::DragFloat3("translate", &transform_.translate.x, 0.1f);
	ImGui::DragInt("HP", &hp_);

	ImGui::End();

#endif // _DEBUG
}

void Player::Attack() {

	// 撃てない状態なら何もしない
	if (!canShoot_) {
		return;
	}

	// --- 右クリック：単発 --- //
	if (System::GetInput()->TriggerMouse(1)) {
		SpawnBullet();
	}

	// --- 左クリック：長押し連射 --- //
	if (System::GetInput()->PushMouse(1) && autofireTimer_ <= 0.0f) {
		SpawnBullet();
		autofireTimer_ = autofireInterval_;
	}
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

void Player::SpawnBullet() {

	Vector3 muzzlePos = transform_.translate;
	if (hasGunMuzzlePos_) {
		muzzlePos = gunMuzzlePos_;
	}

	// ★ シーンと同様：座標セットして Update() で Emit させる
	if (muzzleEmitter_) {
		muzzleEmitter_->SetTranslate(muzzlePos);
		muzzleEmitter_->Update();  // Update の中で Emit() が呼ばれる
	}

	// 弾の見た目（Object3d）を新規作成
	Object3d* bulletObject = new Object3d();
	bulletObject->Init(BlendType::BLEND_NONE);
	bulletObject->SetModel("sphere.obj");
	bulletObject->SetDefaultCamera(camera_);

	auto newBullet = std::make_unique<PlayerBullet>();
	newBullet->Init(camera_, bulletObject);

	// ★ 元に戻す：プレイヤー（カメラ追従中）の位置から発射
	Vector3 spawnPos = transform_.translate;
	newBullet->SetTranlate(spawnPos);

	Vector3 direction;

	if (reticleSprite_) {
		// レティクル方向をレイで計算（ここは今のまま）
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

	if (collisionManager_) {
		collisionManager_->Register(newBullet.get());
	}

	bulletObjects_.emplace_back(std::move(newBullet));
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
			/*Damage(1);
			SetInvincible(true);*/
		}
		break;

	default:
		break;
	}
}