#pragma once
#include <memory>
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/lib/Math/MyMath.h"

class Camera;

class BossSword {

	enum class Mode { kIdle, kProjectile, kSweep };

public:
	BossSword() = default;
	~BossSword() = default;


	void Init(Camera* cam);
	void Spawn(const Vector3& start, const Vector3& target, float speed, int hp = 3);
	void Update();
	void Draw();

	bool OnHitByBullet();

	void StartSweep(const Vector3& center,
		const Vector3& right,
		const Vector3& forward,
		float halfLen,       // 片側の長さ（全長は×2）
		float towardDist,    // 斬りながら手前に寄せる距離
		float duration);     // 斬り時間（秒）

	Mode GetMode() const { return mode_; }

	void ReflectTo(const Vector3& dir, float speed); // パリィ反射で使う

	// 状態取得
	bool IsAlive() const { return alive_; }
	bool IsBroken() const { return broken_; }

	// 参照系
	float GetRadius() const { return radius_; }
	const Vector3& GetPos() const { return pos_; }
	Object3d* GetObj() const { return obj_.get(); }

	// 調整
	void SetScale(const Vector3& s);
	void SetRadius(float r);

private:
	Camera* camera_ = nullptr;
	std::unique_ptr<Object3d> obj_ = nullptr;


	Vector3 pos_{};
	Vector3 vel_{};
	Vector3 rot_{};


	float radius_ = 1.0f;
	int hp_ = 3;
	bool alive_ = false;
	bool broken_ = false;
	float timer_ = 0.f;

	Mode   mode_ = Mode::kIdle;
	float  sweepT_ = 0.f;
	float  sweepDur_ = 0.6f;
	float  sweepHalfLen_ = 8.f;
	float  sweepToward_ = 2.f;
	Vector3 sweepCenter_{};
	Vector3 sweepRight_{};
	Vector3 sweepForward_{};
};