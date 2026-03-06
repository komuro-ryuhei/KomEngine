#include "AssetPreloader.h"
#include "Engine/Base/TextureManager/TextureManager.h"
#include "Engine/Base/ModelManager/ModelManager.h"

void AssetPreloader::Clear() {
    textures_.clear();
    models_.clear();
    texSet_.clear();
    modelSet_.clear();
    texIndex_ = 0;
    modelIndex_ = 0;
    started_ = false;
}

void AssetPreloader::AddTexture(const std::string& path) {
    if (texSet_.insert(path).second) {
        textures_.push_back(path);
    }
}
void AssetPreloader::AddModel(const std::string& name) {
    if (modelSet_.insert(name).second) {
        models_.push_back(name);
    }
}

void AssetPreloader::BuildListFor(const std::string& sceneName) {
    Clear();

    if (sceneName == "TEST") {
        // --- BossTestSceneで使うテクスチャ/モデル（例） ---
        AddTexture("./Resources/images/uvChecker.png");
        AddTexture("./Resources/images/circle.png");
        AddTexture("./Resources/images/circle2.png");
        AddTexture("./Resources/images/monsterBall.png");
        AddTexture("./Resources/images/gradationLine.png");
        AddTexture("./Resources/images/moonLight.png");
        AddTexture("./Resources/images/test.dds");
        AddTexture("./Resources/images/ground.png");
        AddTexture("./Resources/images/reticle.png");
        AddTexture("./Resources/images/inner.png");
        AddTexture("./Resources/images/outer.png");
        AddTexture("./Resources/images/hp.png");
        AddTexture("./Resources/images/blackBG.png");
        AddTexture("./Resources/images/gameClear.png");
        AddTexture("./Resources/images/controlsGuide.png");
        AddTexture("./Resources/images/toTitle.png");
        AddTexture("./Resources/images/returnGame.png");
        AddTexture("./Resources/images/pause.png");
        AddTexture("./Resources/images/toPause.png");
        AddTexture("./Resources/images/heart.png");
        AddTexture("./Resources/images/gauge.png");
        AddTexture("./Resources/images/mouseLeftClickWithText.png");
        AddTexture("./Resources/images/mouseLightClickWithText.png");
        AddTexture("./Resources/images/BossArmor.png");
        AddTexture("./Resources/images/BossEnemyCore.png");
        AddTexture("./Resources/images/PlayerBullet.png");
        AddTexture("./Resources/images/BossEnemyMissile.png");
        AddTexture("./Resources/images/BossEnemyMeteor.png");

        AddModel("plane.obj");
        AddModel("sphere.obj");
        AddModel("axis.obj");
        AddModel("cube.obj");
        AddModel("Player.obj");
        AddModel("Enemy.obj");
        AddModel("ground.obj");
        AddModel("hand.obj");
        AddModel("BossEnemy.obj");
        AddModel("gun.obj");
        AddModel("BossArmor.obj");
        AddModel("BossEnemyCore.obj");
        AddModel("PlayerBullet.obj");
        AddModel("BossEnemyMissile.obj");
        AddModel("BossEnemyMeteor.obj");
    }

    // TITLE側の必要物も入れるなら同様に else if (sceneName=="TITLE") { ... }
}

void AssetPreloader::Start() {
    started_ = true;
    texIndex_ = 0;
    modelIndex_ = 0;
}

void AssetPreloader::Update(int budgetPerFrame) {
    if (!started_) return;

    int budget = budgetPerFrame;

    while (budget > 0 && texIndex_ < textures_.size()) {
        TextureManager::GetInstance()->LoadTexture(textures_[texIndex_++]);
        --budget;
    }
    while (budget > 0 && modelIndex_ < models_.size()) {
        ModelManager::GetInstance()->LoadModel(models_[modelIndex_++]);
        --budget;
    }
}

bool AssetPreloader::IsDone() const {
    return started_
        && texIndex_ >= textures_.size()
        && modelIndex_ >= models_.size();
}

float AssetPreloader::GetProgress() const {
    const float total = float(textures_.size() + models_.size());
    if (total <= 0.0f) return 1.0f;
    const float done = float(texIndex_ + modelIndex_);
    return done / total;
}