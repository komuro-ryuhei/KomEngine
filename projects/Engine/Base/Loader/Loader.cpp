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

		if (type.compare("MESH") == 0) {
			levelData->objects.emplace_back(LevelData::JsonObjectData{});
			LevelData::JsonObjectData& objectData = levelData->objects.back();

			std::string name = object.contains("name") ? object["name"].get<std::string>() : "";

			if (name.find("Cube") != std::string::npos) {
				objectData.fileName = "axis.obj";
			} else if (name.find("Sphere") != std::string::npos) {
				objectData.fileName = "sphere.obj";
			} else if (name.find("Plane") != std::string::npos) {
				objectData.fileName = "plane.obj";
			} else {
				objectData.fileName = "default.obj";
			}

			nlohmann::json& transform = object["transform"];
			objectData.translate.x = (float)transform["translation"][0];
			objectData.translate.y = (float)transform["translation"][1];
			objectData.translate.z = (float)transform["translation"][2];

			objectData.rotate.x = -(float)transform["rotation"][0];
			objectData.rotate.y = -(float)transform["rotation"][1];
			objectData.rotate.z = -(float)transform["rotation"][2];

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