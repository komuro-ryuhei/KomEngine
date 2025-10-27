#pragma once
#include <memory>
#include <algorithm>
#include "Engine/Base/2d/Sprite/Sprite.h"

/// <summary>
/// フェードクラス
/// </summary>
class Fade {
public:
	enum class Status { None, FadeIn, FadeOut };

	enum class Mode { kAlpha, kSlash };

	// 画面サイズはウィンドウ解像度に合わせる
	void Initialize(int screenWidth, int screenHeight,
		const char* texturePath = "./Resources/images/uvChecker.png");

	// フェード開始（durationSec秒）
	void Start(Status status, float durationSec);

	// フェード停止（非表示＆計算停止）
	void Stop();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	// 進行が完了したか（FadeIn / FadeOutとも）
	bool IsFinished() const;

	// フェード中かどうか
	bool IsActive() const { return status_ != Status::None; }

	// スラッシュオープン開始
	void StartSlashOpen(float durationSec, float angleDeg = 45.0f, bool withFlash = true);

private:
	std::unique_ptr<Sprite> sprite_;

	Status status_ = Status::None;
	Mode  mode_ = Mode::kAlpha;

	// 共通
	float  duration_ = 0.0f;   // 継続時間[sec]
	float  counter_ = 0.0f;   // 経過時間[sec]
	int    w_ = 1280, h_ = 720;

	// スラッシュ用
	std::unique_ptr<Sprite> sliceA_, sliceB_; // 黒い覆いを半分ずつ
	std::unique_ptr<Sprite> flash_;           // 斬撃の光（加算）
	float angleRad_ = 0.0f;                   // 切り口角度
	bool  withFlash_ = true;
	float flashTime_ = 0.0f;
	const float flashLife_ = 0.12f;           // 光の残像時間(秒)
};