#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include "externals/nlohmann/json.hpp"

class AssetPreloader {

public:

    // 
    void BuildListFor(const std::string& sceneName);
    // 外部ファイルから対象シーンのロード一覧を構築
    bool BuildListFromJson(const std::string& jsonPath, const std::string& sceneName);

    void Start();

    // 1フレームに決まった数（budget個）だけロードする
    void Update(int budgetPerFrame);

    void Clear();

    bool IsDone() const;
    float GetProgress() const;

    bool BuildCommonListFromJson(const std::string& jsonPath);

private:

    std::vector<std::string> textures_;
    std::vector<std::string> models_;

    size_t texIndex_ = 0;
    size_t modelIndex_ = 0;
    bool started_ = false;

    // 重複登録防止
    std::unordered_set<std::string> texSet_;
    std::unordered_set<std::string> modelSet_;

    void AddTexture(const std::string& path);
    void AddModel(const std::string& name);
};