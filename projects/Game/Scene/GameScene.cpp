#include "GameScene.h"

#include "Engine/Base/System/System.h"
#include "externals/imgui/imgui.h"

void GameScene::Init() {

	// テクスチャの読み込み
	const std::string& uvTexture = "./Resources/images/uvChecker.png";
	TextureManager::GetInstance()->LoadTexture(uvTexture);
	const std::string& monsterBallTexture = "./Resources/images/monsterBall.png";
	TextureManager::GetInstance()->LoadTexture(monsterBallTexture);

	// Sprite
	sprite_ = std::make_unique<Sprite>();
	sprite_->Init(uvTexture);

	object3d_ = std::make_unique<Object3d>();
	object3d_->Init();

	glassObject_ = std::make_unique<Object3d>();
	glassObject_->Init();

	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	ModelManager::GetInstance()->LoadModel("terrain.obj");

	object3d_->SetModel("sphere.obj");
	glassObject_->SetModel("terrain.obj");

	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({0.5f, 0.0f, 0.0f});
	camera_->SetTranslate({0.0f, 7.0f, -12.0f});

	object3d_->SetDefaultCamera(camera_.get());
	glassObject_->SetDefaultCamera(camera_.get());

	audio_ = std::make_unique<Audio>();
	audio_->Init();

	SoundData soundData = audio_->SoundLoadWave("Resources/fanfare.wav");
	// audio_->SoundPlayWave(audio_->GetXAudio2(), soundData);

	//
	ParticleManager::GetInstance()->Init(camera_.get());
	ParticleManager::GetInstance()->CreateParticleGeoup("normal", uvTexture);
	ParticleManager::GetInstance()->CreateParticleGeoup("monsterBall", monsterBallTexture);
	emitter_ = std::make_unique<ParticleEmitter>();
	emitter_->Init("normal", {0.0f, 0.0f, 10.0f}, 50);

	emitter2_ = std::make_unique<ParticleEmitter>();
	emitter2_->Init("monsterBall", {0.0f, 0.0f, 10.0f}, 50);
}

void GameScene::Update() {

	// Sprite描画前処理
	// sprite_->PreDraw();

	camera_->Update();

	sprite_->Update();

	object3d_->Update();
	glassObject_->Update();

	sprite_->ImGuiDebug();

	//
	ParticleManager::GetInstance()->Update();

	if (ImGui::Button("Emit Particles")) {
		emitter_->Update();
	}
	if (ImGui::Button("Emit2 Particles")) {
		emitter2_->Update();
	}

	// 
	camera_->ImGuiDebug();
	// object3d_->ImGuiDebug();
	glassObject_->ImGuiDebug();
}

void GameScene::Draw() {

	// sprite_->Draw();

	object3d_->Draw();
	glassObject_->Draw();

	ParticleManager::GetInstance()->Draw();
}

void GameScene::Finalize() { ParticleManager::GetInstance()->Finalize(); }