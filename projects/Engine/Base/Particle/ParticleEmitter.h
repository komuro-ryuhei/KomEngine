#pragma once

// MyClass
#include "Engine/lib/Math/MyMath.h"
#include "ParticleManager.h"

#include <string>

/// <summary>
/// パーティクル生成器クラス
/// </summary>
class ParticleEmitter {

public:

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="name"> 名前 </param>
	/// <param name="translate"> 座標 </param>
	/// <param name="count"> 生成数 </param>
	void Init(const std::string& name, Vector3 translate, uint32_t count);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 生成
	/// </summary>
	void Emit();

public:

	void SetTranslate(const Vector3& translate) { translate_ = translate; } // 座標のセット

private:
	std::string name_;

	Vector3 translate_;

	uint32_t count_;

	const float kDeltaTime = 1.0f / 60.0f;

	float frequency;
	float frequencyTime;
};