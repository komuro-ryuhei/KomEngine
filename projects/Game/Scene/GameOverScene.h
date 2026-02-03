#pragma once

// Scene
#include "Game/Scene/IScene.h"
#include "Game/Scene/SceneManager.h"

#include "Engine/Base/Camera/Camera.h"
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/2d/Sprite/Sprite.h"
#include "Game/UI/Fade.h" 

/// <summary>
/// ゲームオーバーシーン
/// </summary>
class GameOverScene : public IScene {

public:

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw() override;

private:

	// カメラ
	std::unique_ptr<Camera> camera_ = nullptr;

	// スプライト
	std::unique_ptr<Sprite> sprite_ = nullptr;

	// 倒れているプレイヤーオブジェクト
	std::unique_ptr<Object3d> downPlayer_ = nullptr;

	Vector3 downPlayerRotate_ = { 0.0f, 0.0f, 0.0f };

	// フェード
	std::unique_ptr<Fade> fade_ = nullptr;
	enum class Phase { kFadeIn, kMain, kFadeOut };
	Phase phase_ = Phase::kFadeIn;
};