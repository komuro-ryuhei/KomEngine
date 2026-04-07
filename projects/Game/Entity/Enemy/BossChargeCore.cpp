#include "BossChargeCore.h"
#include "Game/Entity/Player/PlayerBullet.h"
#include "Engine/Base/System/System.h"

void BossChargeCore::Init(Camera* camera) {

	camera_ = camera;

	// 芯
	coreObj_ = std::make_unique<Object3d>();
	coreObj_->Init("object3d_chargeCore", BlendType::BLEND_NONE);
	coreObj_->SetModel("EnemyChargeCore.obj");
	coreObj_->SetDefaultCamera(camera_);
	coreObj_->SetScale(coreScale_);
	coreObj_->SetColor({ 0.82f, 0.93f, 1.0f, 1.0f });
}

void BossChargeCore::Update(float dt) {

	if (!active_) {
		return;
	}

	// -----------------------------
	// 崩壊演出
	// -----------------------------
	if (collapseStarted_) {

		collapseTimer_ += dt;

		if (!coreObj_) {
			return;
		}

		coreObj_->SetTranslate(worldPos_);
		coreObj_->SetRotate({ 0.0f, rotY_, 0.0f });

		switch (collapsePhase_) {

		case CollapsePhase::Flash:
		{
			float t = std::clamp(collapseTimer_ / collapseFlashTime_, 0.0f, 1.0f);

			// 一瞬だけ膨らみつつ白く光る
			float flashScale = 1.0f + 0.35f * std::sin(t * 3.1415926f);

			coreObj_->SetScale({
				coreScale_.x * flashScale,
				coreScale_.y * flashScale,
				coreScale_.z * flashScale
				});

			coreObj_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
			coreObj_->Update();

			if (collapseTimer_ >= collapseFlashTime_) {
				collapsePhase_ = CollapsePhase::Shrink;
				collapseTimer_ = 0.0f;
			}
			return;
		}

		case CollapsePhase::Shrink:
		{
			float t = std::clamp(collapseTimer_ / collapseShrinkTime_, 0.0f, 1.0f);

			// ギュッと内側へ縮む
			float ease = 1.0f - (t * t * (3.0f - 2.0f * t));
			float sc = 0.08f + 0.92f * ease;

			coreObj_->SetScale({
				coreScale_.x * sc,
				coreScale_.y * sc,
				coreScale_.z * sc
				});

			// 白→青白へ戻りつつ薄くなる
			Vector4 c{
				0.75f + 0.25f * (1.0f - t),
				0.90f + 0.10f * (1.0f - t),
				1.00f,
				1.0f - 0.35f * t
			};

			coreObj_->SetColor(c);
			coreObj_->Update();

			if (collapseTimer_ >= collapseShrinkTime_) {
				collapsePhase_ = CollapsePhase::Burst;
				collapseTimer_ = 0.0f;
			}
			return;
		}

		case CollapsePhase::Burst:
		{
			float t = std::clamp(collapseTimer_ / collapseBurstTime_, 0.0f, 1.0f);

			// 縮んだあと、少しだけ弾けるように広がって消える
			float sc = 0.08f + 0.28f * t;

			coreObj_->SetScale({
				coreScale_.x * sc,
				coreScale_.y * sc,
				coreScale_.z * sc
				});

			Vector4 c{
				0.68f,
				0.88f,
				1.00f,
				1.0f - t
			};

			coreObj_->SetColor(c);
			coreObj_->Update();

			if (collapseTimer_ >= collapseBurstTime_) {
				collapsePhase_ = CollapsePhase::Done;
				active_ = false;
			}
			return;
		}

		case CollapsePhase::Done:
		default:
			active_ = false;
			return;
		}
	}

	// -----------------------------
	// 通常時（チャージ中）
	// -----------------------------
	pulseTime_ += dt;

	// 回転はかなり弱くする
	rotY_ += dt * 0.04f;

	float hpRate = (maxHp_ > 0) ? static_cast<float>(hp_) / static_cast<float>(maxHp_) : 0.0f;
	hpRate = std::clamp(hpRate, 0.0f, 1.0f);

	// チャージ経過でどんどん大きくする
	const float growDuration = 3.5f;
	float growT = std::clamp(pulseTime_ / growDuration, 0.0f, 1.0f);

	// 後半ほど迫ってくる感じ
	float growEase = growT * growT * (3.0f - 2.0f * growT);

	// 最小倍率 -> 最大倍率
	float growScale = 0.72f + (1.65f - 0.72f) * growEase;

	// HPが減っている時は少し不安定さを足す
	float unstable = (1.0f - hpRate) * 0.04f * std::sin(pulseTime_ * 14.0f);

	float finalScale = growScale + unstable;

	Vector4 coreColor{
		0.86f + 0.10f * growT,
		0.94f + 0.03f * growT,
		1.00f,
		1.0f
	};

	if (coreObj_) {
		coreObj_->SetTranslate(worldPos_);
		coreObj_->SetRotate({ 0.0f, rotY_, 0.0f });
		coreObj_->SetScale({
			coreScale_.x * finalScale,
			coreScale_.y * finalScale,
			coreScale_.z * finalScale
			});
		coreObj_->SetColor(coreColor);
		coreObj_->Update();
	}
}

void BossChargeCore::Draw() {

	if (!active_) {
		return;
	}

	if (coreObj_) {
		coreObj_->Draw();
	}
}

void BossChargeCore::Activate(const Vector3& worldPos) {

	active_ = true;
	hp_ = maxHp_;
	worldPos_ = worldPos;
	pulseTime_ = 0.0f;
	rotY_ = 0.0f;

	if (coreObj_) {
		coreObj_->SetTranslate(worldPos_);
		coreObj_->SetScale(coreScale_);
	}
}

void BossChargeCore::Deactivate() {
	active_ = false;
}

void BossChargeCore::StartCollapse() {

	collapseStarted_ = true;
	collapsePhase_ = CollapsePhase::Flash;
	collapseTimer_ = 0.0f;
	brokenJustNow_ = true;
}

bool BossChargeCore::ConsumeBrokenJustNow() {

	if (!brokenJustNow_) {
		return false;
	}
	brokenJustNow_ = false;
	return true;
}

void BossChargeCore::SetWorldPos(const Vector3& worldPos) {

	worldPos_ = worldPos;

	if (coreObj_) {
		coreObj_->SetTranslate(worldPos_);
	}
}

void BossChargeCore::OnCollision(ICollisionObject* other) {

	if (!active_) {
		return;
	}

	if (collapseStarted_) {
		return;
	}

	if (other->GetCollisionLayer() != CollisionLayer::PlayerBullet) {
		return;
	}

	int dmg = 1;
	if (auto* bullet = dynamic_cast<PlayerBullet*>(other)) {
		dmg = std::max(1, bullet->GetDamage());
	}

	hp_ -= dmg;
	if (hp_ < 0) {
		hp_ = 0;
	}

	if (hp_ <= 0) {
		StartCollapse();
	}
}