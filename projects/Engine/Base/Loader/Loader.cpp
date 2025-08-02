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
	if (file.fail()) {
		assert(0);
	}

	nlohmann::json deserialized;
	file >> deserialized;

	assert(deserialized.is_object());
	assert(deserialized.contains("name"));
	assert(deserialized["name"].is_string());

	std::string name = deserialized["name"].get<std::string>();
	assert(name.compare("scene") == 0);

	levelData = new LevelData();

	for (nlohmann::json& object : deserialized["objects"]) {
		assert(object.contains("type"));
		std::string type = object["type"].get<std::string>();

		// visible が false ならスキップ
		if (object.contains("visible") && object["visible"].is_boolean()) {
			if (object["visible"].get<bool>() == false) {
				continue;
			}
		}

		if (type.compare("MESH") == 0) {
			levelData->objects.emplace_back(LevelData::JsonObjectData{});
			LevelData::JsonObjectData& objectData = levelData->objects.back();

			std::string name = object.contains("name") ? object["name"].get<std::string>() : "";

			if (name.find("Cube") != std::string::npos) {
				objectData.fileName = "cube.obj";
			} else if (name.find("Sphere") != std::string::npos) {
				objectData.fileName = "sphere.obj";
			} else if (name.find("Plane") != std::string::npos) {
				objectData.fileName = "plane.obj";
			} else if (name.find("Player") != std::string::npos) {
				objectData.fileName = "Player.obj";
			} else if (name.find("Enemy") != std::string::npos) {
				objectData.fileName = "Enemy.obj";
			} else {
				objectData.fileName = "default.obj";
			}

			// トランスフォームの読み込み
			nlohmann::json& transform = object["transform"];

			// Blender → 自作エンジン座標変換（位置）
			objectData.translate.x = (float)transform["translation"][0];
			objectData.translate.y = (float)transform["translation"][2];
			objectData.translate.z = (float)transform["translation"][1];

			// Blender → 自作エンジン座標変換（回転）
			constexpr float ToRadians = 3.14159265f / 180.0f;

			objectData.rotate.x = -(float)transform["rotation"][0] * ToRadians;
			objectData.rotate.y = -(float)transform["rotation"][2] * ToRadians;
			objectData.rotate.z = (float)transform["rotation"][1] * ToRadians;

			// 
			objectData.scale.x = (float)transform["scaling"][0];
			objectData.scale.y = (float)transform["scaling"][1];
			objectData.scale.z = (float)transform["scaling"][2];
		}
	}

	for (auto& objectData : levelData->objects) {
		Model* model = nullptr;
		auto it = models.find(objectData.fileName);
		if (it != models.end()) {
			model = it->second;
		}

		Object3d* newObject = new Object3d();
		newObject->Init(BlendType::BLEND_NONE);
		newObject->SetTranslate(objectData.translate);
		newObject->SetRotate(objectData.rotate);
		newObject->SetScale(objectData.scale);
		newObject->SetFromBlender(true);

		if (!objectData.fileName.empty()) {
			newObject->SetModel(objectData.fileName);
		} else {
			newObject->SetModel("sphere.obj");
		}
		newObject->SetDefaultCamera(camera);
		objects.push_back(newObject);
	}
}

void Loader::Update() {
	for (auto& object : objects) {
		object->Update();
	}
}

void Loader::Draw() {
	for (auto& object : objects) {
		object->Draw();
	}
}

void Loader::Clear() {
	for (auto& object : objects) {
		delete object;
	}
	objects.clear();

	if (levelData) {
		delete levelData;
		levelData = nullptr;
	}
}

void Loader::Reload(Camera* camera) {
	Clear();
	Init(camera);
}