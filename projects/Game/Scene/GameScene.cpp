#include "GameScene.h"

#include "Engine/Base/System/System.h"

#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif // DEBUG

void GameScene::Init() {

	// テクスチャの読み込み
	const std::string& uvTexture = "./Resources/images/uvChecker.png";
	const std::string& circle = "./Resources/images/circle.png";
	const std::string& circle2 = "./Resources/images/circle2.png";
	const std::string& monsterBallTexture = "./Resources/images/monsterBall.png";
	const std::string& ring = "./Resources/images/gradationLine.png";
	const std::string& moonLight = "./Resources/images/moonLight.png";

	TextureManager::GetInstance()->LoadTexture(uvTexture);
	TextureManager::GetInstance()->LoadTexture(circle);
	TextureManager::GetInstance()->LoadTexture(circle2);
	TextureManager::GetInstance()->LoadTexture(monsterBallTexture);
	TextureManager::GetInstance()->LoadTexture("./Resources/images/rostock_laage_airport_4k.dds");

	// Skybox
	skybox_ = std::make_unique<Skybox>();
	skybox_->Init("./Resources/images/rostock_laage_airport_4k.dds");

	// Sprite
	sprite_ = std::make_unique<Sprite>();
	sprite_->Init(uvTexture, BlendType::BLEND_NONE);

	object3d_ = std::make_unique<Object3d>();
	object3d_->Init(BlendType::BLEND_NONE);

	glassObject_ = std::make_unique<Object3d>();
	glassObject_->Init(BlendType::BLEND_NONE);

	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	ModelManager::GetInstance()->LoadModel("terrain.obj");

	object3d_->SetModel("sphere.obj");
	glassObject_->SetModel("terrain.obj");

	camera_ = std::make_unique<Camera>();
	// camera_->SetRotate({0.2f, 0.0f, 0.0f});
	// camera_->SetTranslate({0.0f, 7.0f, -30.0f});
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 0.0f, -12.0f });

	object3d_->SetDefaultCamera(camera_.get());
	glassObject_->SetDefaultCamera(camera_.get());

	audio_ = std::make_unique<Audio>();
	audio_->Init();

	SoundData soundData = audio_->SoundLoadWave("Resources/fanfare.wav");
	// audio_->SoundPlayWave(audio_->GetXAudio2(), soundData);

	//
	ParticleManager::GetInstance()->Init(camera_.get(), BlendType::BLEND_ADD);
	ParticleManager::GetInstance()->CreateParticleGeoup("hit", circle2, "a");
	ParticleManager::GetInstance()->CreateParticleGeoup("explosion", monsterBallTexture, "a");
	ParticleManager::GetInstance()->CreateParticleGeoup("ring", ring, "ring");
	ParticleManager::GetInstance()->CreateParticleGeoup("cylinder", ring, "cylinder");
	ParticleManager::GetInstance()->CreateParticleGeoup("moonLight", moonLight, "moonLight");
	ParticleManager::GetInstance()->CreateParticleGeoup("ribbon", moonLight, "ribbon");

	emitter_ = std::make_unique<ParticleEmitter>();
	emitter_->Init("hit", { 0.0f, 0.0f, 10.0f }, 8);

	emitter2_ = std::make_unique<ParticleEmitter>();
	emitter2_->Init("explosion", { 0.0f, 0.0f, 10.0f }, 50);

	ringEmitter_ = std::make_unique<ParticleEmitter>();
	ringEmitter_->Init("ring", { 0.0f, 0.0f, 10.0f }, 1);

	cylinderEmitter_ = std::make_unique<ParticleEmitter>();
	cylinderEmitter_->Init("cylinder", { 0.0f, 0.0f, 10.0f }, 1);

	moonLightEffect_ = std::make_unique<ParticleEmitter>();
	moonLightEffect_->Init("moonLight", { 0.0f, 0.0f, 10.0f }, 1);

	ribbonEffect_ = std::make_unique<ParticleEmitter>();
	ribbonEffect_->Init("ribbon", { 0.0f, 0.0f, 10.0f }, 1);
}

void GameScene::Update() {

	// Sprite描画前処理
	// sprite_->PreDraw();

	camera_->Update();

	sprite_->Update();

	skybox_->Update();

	object3d_->Update();
	glassObject_->Update();

	//
	ParticleManager::GetInstance()->Update();
	ParticleUpdate();


#ifdef _DEBUG
	camera_->ImGuiDebug();
	object3d_->ImGuiDebug();
	glassObject_->ImGuiDebug();
	sprite_->ImGuiDebug();
#endif // _DEBUG
}

void GameScene::Draw() {

	// 
	skybox_->Draw();

	// sprite_->Draw();

	//
	// object3d_->Draw();

	//// 地面
	// glassObject_->Draw();

	ParticleManager::GetInstance()->Draw();
}

void GameScene::Finalize() { ParticleManager::GetInstance()->Finalize(); }

void GameScene::ParticleUpdate() {

#ifdef _DEBUG

	if (ImGui::Button("Emit Particles")) {
		emitter_->Update();
	}

	if (ImGui::Button("Emit2 Particles")) {
		emitter2_->Update();
	}

	if (ImGui::Button("Ring Particles")) {
		ringEmitter_->Update();
	}

	if (ImGui::Button("Cylinder Particles")) {
		cylinderEmitter_->Update();
	}

	if (ImGui::Button("MoonLight Effect")) {
		moonLightEffect_->Update();
	}

	if (ImGui::Button("Ribbon Effect")) {
		ribbonEffect_->Update();
	}

	if (System::TriggerKey(DIK_1)) {
		emitter_->Update();
	}
	if (System::TriggerKey(DIK_2)) {
		moonLightEffect_->Update();
	}
	if (System::TriggerKey(DIK_3)) {
		ribbonEffect_->Update();
	}

#endif // _DEBUG

	// ribbonEffect_->Update();

	// ringEmitter_->Update();
	// cylinderEmitter_->Update();
}