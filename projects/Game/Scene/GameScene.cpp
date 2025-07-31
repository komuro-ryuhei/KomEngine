#include "GameScene.h"

#include "Engine/Base/System/System.h"
#include "externals/nlohmann/json.hpp"


#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif // DEBUG

GameScene::GameScene() {}
GameScene::~GameScene() {}

void GameScene::Init() {

	camera_ = std::make_unique<Camera>();
	// camera_->SetRotate({0.2f, 0.0f, 0.0f});
	// camera_->SetTranslate({0.0f, 7.0f, -30.0f});
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 3.0f, -30.0f });

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
	TextureManager::GetInstance()->LoadTexture("./Resources/images/test.dds");
	TextureManager::GetInstance()->LoadTexture("./Resources/images/ground.png");

	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	ModelManager::GetInstance()->LoadModel("terrain.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");
	ModelManager::GetInstance()->LoadModel("Player.obj");
	ModelManager::GetInstance()->LoadModel("Enemy.obj");
	ModelManager::GetInstance()->LoadModel("ground.obj");

	// Skybox
	skybox_ = std::make_unique<Skybox>();
	skybox_->Init("./Resources/images/test.dds");
	skybox_->SetDefaultCamera(camera_.get());

	// Sprite
	sprite_ = std::make_unique<Sprite>();
	sprite_->Init(uvTexture, BlendType::BLEND_NONE);

	object3d_ = std::make_unique<Object3d>();
	object3d_->Init(BlendType::BLEND_NONE);
	object3d_->SetModel("sphere.obj");
	object3d_->SetDefaultCamera(camera_.get());
	object3d_->SetEnvironmentTexture("./Resources/images/test.dds");

	glassObject_ = std::make_unique<Object3d>();
	glassObject_->Init(BlendType::BLEND_NONE);
	glassObject_->SetModel("ground.obj");
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

	loader_ = std::make_unique<Loader>();
	loader_->Init(camera_.get());
	ribbonEffect_->Init("ribbon", { 0.0f, 0.0f, 10.0f }, 1);

	// **ユーザー認証**
	// rankingManager.Login(L"komuro", L"password");
}

void GameScene::Update() {

	camera_->Update();

	sprite_->Update();

	skybox_->Update();

	object3d_->Update();
	glassObject_->Update();

	loader_->Update();

	//
	ParticleManager::GetInstance()->Update();

#ifdef _DEBUG

	// **ImGuiのデバッグ描画**
	ImGuiDebug();

	// Particle描画ImGui
	ParticleUpdate();

	// **ランキングの描画**
	// rankingManager.Render();

#endif // _DEBUG
}

void GameScene::Draw() {

	// 
	skybox_->Draw();

	// sprite_->Draw();

	//
	object3d_->Draw();

	// 地面
	glassObject_->Draw();

	// loader_->Draw();

	ParticleManager::GetInstance()->Draw();
}

void GameScene::Finalize() { ParticleManager::GetInstance()->Finalize(); }

void GameScene::ImGuiDebug() {

#ifdef _DEBUG

	ChangePostEffect();

	camera_->ImGuiDebug(); // カメラ
	object3d_->ImGuiDebug(); // オブジェクト
	sprite_->ImGuiDebug(); // スプライト
	skybox_->ImGuiDebug(); // スカイボックス

	if (ImGui::Button("Reload Scene JSON")) {
		loader_->Reload(camera_.get());
	}

#endif // _DEBUG
}

void GameScene::ParticleUpdate() {

#ifdef _DEBUG

	ImGui::Begin("Particle Emitter");

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

	ImGui::End();

#endif // _DEBUG

	// ribbonEffect_->Update();

	// ringEmitter_->Update();
	// cylinderEmitter_->Update();
}

void GameScene::ChangePostEffect() {

#ifdef _DEBUG
	// ポストエフェクトの選択肢
	static const char* effectItems[] = {
		"None", "Grayscale", "Vignetting", "Smoothing", "GaussinanFilter", "RadialBlur", "Random","Outline",
		"Glitch","Pixel", "ChromaticAberration", "VHSNoise","ColorInversion",
	};

	ImGui::Begin("PostEffect Settings");
	if (ImGui::Combo("Post Effect", &selectedPostEffectIndex_, effectItems, IM_ARRAYSIZE(effectItems))) {
		// エフェクト名を取得
		std::string selectedEffect = effectItems[selectedPostEffectIndex_];

		if (selectedEffect == "None") {
			System::GetOffscreenRendering()->SetPostEffect("none");
		} else {
			System::GetOffscreenRendering()->SetPostEffect(selectedEffect);
		}
	}
	ImGui::End();
#endif
}