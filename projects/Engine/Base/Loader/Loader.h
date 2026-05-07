#pragma once

#include "Engine/Base/3d/Model/Model.h"
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/lib/Math/MyMath.h"
#include "externals/nlohmann/json.hpp"
#include <map>
#include <memory>

/// <summary>
/// レベルローダークラス
/// </summary>
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

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="camera"> カメラ </param>
	void Init(Camera* camera);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// ホットリロード
	/// </summary>
	/// <param name="camera"> カメラ </param>
	void Reload(Camera* camera);
private:

	/// <summary>
	/// オブジェクト削除関数
	/// </summary>
	void Clear();

private:

	// レベルデータ格納用インスタンスを生成
	std::unique_ptr<LevelData> levelData_;

	std::vector<std::unique_ptr<Object3d>> objects_;
	std::map<std::string, Model*> models_;
};