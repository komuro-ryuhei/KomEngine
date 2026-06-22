#include "QRCodeScene.h"

#include <Windows.h>

void QRCodeScene::Init() {

	GenerateQRCode();
}

void QRCodeScene::Update() {

	const bool space = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
	const bool enter = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;

	// Spaceで新しいQRを生成
	if (space && !preSpace_) {
		GenerateQRCode();
	}

	// Enterでタイトルへ戻る
	if (enter && !preEnter_) {
		if (sceneManager_) {
			sceneManager_->ChangeScene("TITLE");
		}
	}

	preSpace_ = space;
	preEnter_ = enter;

	if (qrBackSprite_) {
		qrBackSprite_->Update();
	}

	for (auto& sprite : qrBlackSprites_) {
		if (sprite) {
			sprite->Update();
		}
	}
}

void QRCodeScene::Draw() {

	if (qrBackSprite_) {
		qrBackSprite_->Draw();
	}

	for (auto& sprite : qrBlackSprites_) {
		if (sprite) {
			sprite->Draw();
		}
	}
}

void QRCodeScene::Finalize() {

	qrBlackSprites_.clear();
	qrBackSprite_.reset();
}

void QRCodeScene::GenerateQRCode() {

	// 固定URLに飛ばしたい場合は token は不要
	token_.clear();

	qrUrl_ = "http://192.168.1.13:3000/faculties";

	const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(
		qrUrl_.c_str(),
		qrcodegen::QrCode::Ecc::LOW
	);

	BuildQrSprites(qr);
}

std::string QRCodeScene::CreateRandomToken() {

	static constexpr char chars[] =
		"0123456789"
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<int> dist(0, static_cast<int>(sizeof(chars) - 2));

	std::string token;
	token.reserve(24);

	for (int i = 0; i < 24; ++i) {
		token += chars[dist(mt)];
	}

	return token;
}

void QRCodeScene::BuildQrSprites(const qrcodegen::QrCode& qr) {

	qrBlackSprites_.clear();

	const int qrSize = qr.getSize();

	const float totalSize = static_cast<float>(qrSize + qrMargin_ * 2) * cellSize_;

	// 白背景
	qrBackSprite_ = std::make_unique<Sprite>();
	qrBackSprite_->Init(whiteTexturePath_, BlendType::BLEND_NONE);
	qrBackSprite_->SetPosition(qrPosition_);
	qrBackSprite_->SetSize({ totalSize, totalSize });
	qrBackSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	qrBackSprite_->Update();

	// 黒マスだけSprite化
	for (int y = 0; y < qrSize; ++y) {
		for (int x = 0; x < qrSize; ++x) {
			if (!qr.getModule(x, y)) {
				continue;
			}

			auto sprite = std::make_unique<Sprite>();
			sprite->Init(whiteTexturePath_, BlendType::BLEND_NONE);

			const float drawX = qrPosition_.x + static_cast<float>(x + qrMargin_) * cellSize_;
			const float drawY = qrPosition_.y + static_cast<float>(y + qrMargin_) * cellSize_;

			sprite->SetPosition({ drawX, drawY });
			sprite->SetSize({ cellSize_, cellSize_ });

			// 白画像を黒にして使う
			sprite->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });

			sprite->Update();

			qrBlackSprites_.push_back(std::move(sprite));
		}
	}
}