#include "BossMeteor.h"
#include "Engine/Base/Camera/Camera.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include "Engine/lib/Math/MyMath.h"

void BossMeteor::Init(Camera* camera) {

	camera_ = camera;

	object3d_ = std::make_unique<Object3d>();
	object3d_->Init(BlendType::BLEND_NONE);
	// モデルは仮で sphere。
	object3d_->SetModel("sphere.obj");
	object3d_->SetDefaultCamera(camera_);
	object3d_->SetScale({ 0.9f, 0.9f, 0.9f });

	// 加速度、重力
	accel_ = { 0.0f, -gravity_, 0.0f };
	velocity_ = { 0.0f, 0.0f, 0.0f };

	// 判定半径
	object3d_->SetRadius(radius_ * object3d_->GetScale().x);
}

void BossMeteor::Spawn(const Vector3& startPos, const Vector3& targetPos, float speed) {

	transform_.translate = startPos;
	transform_.rotate = { 0.0f, 0.0f, 0.0f };

	Vector3 dir = targetPos - startPos;
	if (MyMath::Length(dir) > 0.0001f) { dir = MyMath::Normalize(dir); }
	velocity_ = dir * speed;

	lifeTimer_ = 0.0f;
	isExploding_ = false;
	isAlive_ = true;
}

void BossMeteor::Update() {

	if (!isAlive_) return;

	ApplyPhysics();

	// 回転（見た目用）
	transform_.rotate.x += rotateSpeed_;
	transform_.rotate.y += rotateSpeed_ * 0.6f;

	// Object3Dへ反映
	object3d_->SetTranslate(transform_.translate);
	object3d_->SetRotate(transform_.rotate);
	object3d_->Update();

	// セーフティ：寿命
	lifeTimer_ += 1.0f / 60.0f;
	if (lifeTimer_ > maxLife_) {
		isAlive_ = false;
	}
}

void BossMeteor::ApplyPhysics() {

	// 加速度→速度→位置
	velocity_ += accel_;
	transform_.translate += velocity_;

	// 地面に到達したらヒット
	if (transform_.translate.y <= groundY_) {
		OnHitGround();
	}
}

void BossMeteor::OnHitGround() {

	transform_.translate.y = groundY_;
	Explode();
}

void BossMeteor::Explode() {

	// 
	isExploding_ = true;
	isAlive_ = false;
}

void BossMeteor::Draw() {

	if (!isAlive_) return;
	object3d_->Draw();
}

void BossMeteor::ImGuiDebug() {


#ifdef USE_IMGUI

	ImGui::Begin("BossMeteor");
	ImGui::Checkbox("Alive", &isAlive_);
	ImGui::DragFloat3("Pos", &transform_.translate.x, 0.05f);
	ImGui::DragFloat3("Vel", &velocity_.x, 0.01f);
	ImGui::DragFloat("Gravity", &gravity_, 0.001f, 0.0f, 1.0f);
	ImGui::DragFloat("RotateSpd", &rotateSpeed_, 0.001f, 0.0f, 1.0f);
	ImGui::DragFloat("Radius(base)", &radius_, 0.01f, 0.01f, 10.0f);
	ImGui::DragFloat("GroundY", &groundY_, 0.01f, -100.0f, 100.0f);
	ImGui::DragFloat("MaxLife", &maxLife_, 0.01f, 0.0f, 60.0f);
	if (object3d_) {
		Vector3 s = object3d_->GetScale();
		if (ImGui::DragFloat3("Scale", &s.x, 0.01f, 0.01f, 10.0f)) {
			SetScale(s);
		}
	}
	ImGui::End();
#endif
}

Vector3 BossMeteor::GetCollisionPosition() const {

	// Object3d があれば正確なワールド座標を返す
	if (object3d_) {
		return object3d_->GetWorldPosition();
	}
	return transform_.translate;
}

float BossMeteor::GetCollisionRadius() const {

	// 死んでいるメテオは判定 0 にして無効化
	if (!isAlive_) {
		return 0.0f;
	}

	if (object3d_) {
		return object3d_->GetRadius();
	}
	return radius_;
}

void BossMeteor::OnCollision(ICollisionObject* other) {

	if (!isAlive_) return;

	if (other->GetCollisionLayer() == CollisionLayer::Player) {

		auto* player = dynamic_cast<Player*>(other);
		if (player && !player->IsInvincible()) {

			player->Damage(1);
			player->SetInvincible(true);

			Explode();   // メテオを消す
		}
	} else if (other->GetCollisionLayer() == CollisionLayer::PlayerBullet) {
		// 消滅
		Explode();
	}
}