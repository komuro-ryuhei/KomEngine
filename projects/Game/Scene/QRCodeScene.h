#pragma once

#include "Game/Scene/IScene.h"
#include "Game/Scene/SceneManager.h"

#include "Engine/Base/2D/Sprite/Sprite.h"

#include "externals/QRCode/qrcodegen.hpp"

#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

class QRCodeScene : public IScene {

public:

	void Init() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:

	// QRを作り直す
	void GenerateQRCode();

	// ランダムtoken作成
	std::string CreateRandomToken();

	// QRの黒マスSpriteを作る
	void BuildQrSprites(const qrcodegen::QrCode& qr);

private:
	// QRに入れるURL
	std::string qrUrl_;

	// token
	std::string token_;

	// QR背景
	std::unique_ptr<Sprite> qrBackSprite_;

	// QRの黒マス
	std::vector<std::unique_ptr<Sprite>> qrBlackSprites_;

	// 1マスの大きさ
	float cellSize_ = 8.0f;

	// QR表示位置
	Vector2 qrPosition_ = { 420.0f, 100.0f };

	// QRの余白
	int qrMargin_ = 4;

	// 入力の押しっぱなし対策
	bool preSpace_ = false;
	bool preEnter_ = false;

	// 白画像
	const std::string whiteTexturePath_ = "./Resources/images/white_1x1.png";
};