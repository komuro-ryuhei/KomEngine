#include "Loader.h"

#include <cassert>
#include <fstream>

void Loader::Init(Camera* camera) {

	// ファイルパスを得る
	const std::string fullPath = "Resources/json/unnamed.json";

	// ファイルストリーム
	std::ifstream file;

	// ファイルを開く
	file.open(fullPath);
	// ファイルオープン失敗をチェック
	if (file.fail()) {
		assert(0);
	}

	// JSON文字列から解凍したデータ
	nlohmann::json deserialized;
	// 解凍
	file >> deserialized;

	// 正しいレベルデータファイルかチェック
	assert(deserialized.is_object());
	assert(deserialized.contains("name"));
	assert(deserialized["name"].is_string());

	// "name"を文字列として取得
	std::string name = deserialized["name"].get<std::string>();
	// 正しいレベルデータファイルかチェック
	assert(name.compare("scene") == 0);

	// レベルデータ格納用インスタンスを生成
	levelData = new LevelData();

	// "objects"の全オブジェクトを走査
	for (nlohmann::json& object : deserialized["objects"]) {
		assert(object.contains("type"));

		// 種別を取得
		std::string type = object["type"].get<std::string>();

		// 種類ごとの処理

		// MESH
		if (type.compare("MESH") == 0) {
			// 要素追加
			levelData->objects.emplace_back(LevelData::JsonObjectData{});
			// 今追加した要素の参照を得る
			LevelData::JsonObjectData& objectData = levelData->objects.back();

			if (object.contains("file_name")) {
				// ファイル名
				objectData.fileName = object["file_name"];
			}

			// トランスフォームのパラメータ読み込み
			nlohmann::json& transform = object["transform"];
			// 平行移動
			objectData.translate.x = (float)transform["translation"][0];
			objectData.translate.y = (float)transform["translation"][1];
			objectData.translate.z = (float)transform["translation"][2];
			// 回転角
			objectData.rotate.x = -(float)transform["rotation"][0];
			objectData.rotate.y = -(float)transform["rotation"][1];
			objectData.rotate.z = -(float)transform["rotation"][2];
			// スケーリング
			objectData.scale.x = (float)transform["scaling"][0];
			objectData.scale.y = (float)transform["scaling"][1];
			objectData.scale.z = (float)transform["scaling"][2];
		}

		// 再帰処理
		if (object.contains("children")) {
		}
	}

	// レベルデータからオブジェクトを生成、配置
	for (auto& objectData : levelData->objects) {
		// ファイル名から登録済みモデルを検索
		Model* model = nullptr;
		decltype(models)::iterator it = models.find(objectData.fileName);
		if (it != models.end()) {
			// 登録済みモデルがあればそれを使用
			model = it->second;
		}
		//
		Object3d* newObject = new Object3d();
		// 初期化
		newObject->Init(BlendType::BLEND_NONE);
		// 座標
		newObject->SetTranslate(objectData.translate);
		// 回転角
		newObject->SetRotate(objectData.rotate);
		// スケール
		newObject->SetScale(objectData.scale);

		// モデル、カメラの設定
		if (objectData.fileName.empty() == false) {
			newObject->SetModel(objectData.fileName);
		} else {
			newObject->SetModel("sphere.obj");
		}
		newObject->SetDefaultCamera(camera);

		objects.push_back(newObject);
	}
}

void Loader::Update() {

	// 全オブジェクトを更新
	for (auto& object : objects) {
		object->Update();
	}
}

void Loader::Draw() {

	// 全オブジェクトを描画
	for (auto& object : objects) {
		object->Draw();
	}
}