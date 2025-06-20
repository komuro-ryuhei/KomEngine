#pragma once

#include "Engine/Base/3d/Model/Model.h"
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/lib/Math/MyMath.h"
#include "externals/nlohmann/json.hpp"
#include <map>

class Loader {

	struct LevelData {

		struct JsonObjectData {
			Vector3 scale;
			Vector3 rotate;
			Vector3 translate;

			std::string fileName;
		};
		std::vector<JsonObjectData> objects;
	};

public:
	void Init();

	void Update();

	void Draw();

private:

	// レベルデータ格納用インスタンスを生成
	LevelData* levelData = nullptr;

	std::vector<Object3d*> objects;
	std::map<std::string, Model*> models;
};