#include "BossSword.h"
#include "Engine/Base/Camera/Camera.h"

void BossSword::Init(Camera* cam) {
	camera_ = cam;
	obj_ = std::make_unique<Object3d>();
	obj_->Init(BlendType::BLEND_NONE);
	obj_->SetModel("cube.obj");
	obj_->SetDefaultCamera(camera_);
	obj_->SetScale({ 1.2f, 1.2f, 1.2f });

	radius_ = 1.0f;
	obj_->SetRadius(radius_ * obj_->GetScale().x);
}

void BossSword::Spawn(const Vector3& start, const Vector3& target, float speed, int hp) {

	mode_ = Mode::kProjectile;

	pos_ = start;
	Vector3 dir = target - start;
	if (MyMath::Length(dir) > 0.0001f) dir = MyMath::Normalize(dir);
	vel_ = dir * speed;


	hp_ = hp;
	alive_ = true;
	broken_ = false;
	timer_ = 0.f;
}

void BossSword::Update() {

	if (!alive_) return;

	if (mode_ == Mode::kProjectile) {
		pos_ += vel_;
		rot_.y += 0.2f;
	} else if (mode_ == Mode::kSweep) {
		const float dt = KomEngine::System::GetDeltaTime();
		sweepT_ += dt / sweepDur_;
		if (sweepT_ >= 1.f) { alive_ = false; } // 斬り終わりで消滅

		float u = MyMath::Clamp01(sweepT_);
		// 端→端へ直線移動＋少し手前へ寄せる
		// pos = center + right * lerp(-halfLen, +halfLen, u) + forward * (towardDist * u)
		pos_ = sweepCenter_ + sweepRight_ * MyMath::Lerp(-sweepHalfLen_, sweepHalfLen_, u)
			+ sweepForward_ * (sweepToward_ * u);
		// 見た目用の回転（横に流れる感じ）
		rot_.z = 0.25f * std::sin(u * 3.14159f);
	}

	obj_->SetTranslate(pos_);
	obj_->SetRotate(rot_);
	obj_->Update();

	timer_ += 1.f / 60.f;
	if (timer_ > 8.f) alive_ = false;
}

void BossSword::Draw() {

	if (alive_) obj_->Draw();
}

void BossSword::StartSweep(const Vector3& center, const Vector3& right, const Vector3& forward, float halfLen, float towardDist, float duration) {

	mode_ = Mode::kSweep;
	sweepCenter_ = center;
	sweepRight_ = MyMath::Normalize(right);
	sweepForward_ = MyMath::Normalize(forward);
	sweepHalfLen_ = halfLen;
	sweepToward_ = towardDist;
	sweepDur_ = std::max(0.05f, duration);
	sweepT_ = 0.f;

	// 初期位置：左端から開始（右へ払う）
	pos_ = sweepCenter_ - sweepRight_ * sweepHalfLen_;
	vel_ = { 0,0,0 };
	alive_ = true;
	broken_ = false;
	timer_ = 0.f;

	// 剣の向き：前を向ける
	rot_ = { 0, 0, 0 }; // 必要ならここで forward を向く回転に調整
}

void BossSword::ReflectTo(const Vector3& dir, float speed) {
	mode_ = Mode::kProjectile;
	vel_ = MyMath::Normalize(dir) * speed;
	alive_ = true;      // 壊れた扱いではなく、飛んでいく演出
	broken_ = true;     // ただし被弾無効化（プレイヤーにこれ以上当てない）
	timer_ = 0.f;
}

bool BossSword::OnHitByBullet() {

	if (!alive_) return false;
	if (--hp_ <= 0) { alive_ = false; broken_ = true; }
	return !alive_;
}

void BossSword::SetScale(const Vector3& s) {

	if (obj_) {
		obj_->SetScale(s);
		obj_->SetRadius(radius_ * s.x);
	}
}

void BossSword::SetRadius(float r) {

	radius_ = r;
	if (obj_) obj_->SetRadius(radius_ * obj_->GetScale().x);
}