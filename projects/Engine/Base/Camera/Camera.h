#pragma once

#include "Engine/lib/Math/MyMath.h"
#include "struct.h"

enum class CameraShakeType {
	Small,
	Medium,
	Large
};

class Camera {

public:
	Camera();
	~Camera() = default;

	void Init();

	void Update();

	void ImGuiDebug();

public:
	void SetEye(const Vector3& eye);
	void SetTarget(const Vector3& target);

	// シェイク開始
	void StartShake(CameraShakeType type = CameraShakeType::Medium);
	// シェイク処理
	void CameraShake();

public:
	// setter
	void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
	void SetTranslate(const Vector3& tranlate) { transform_.translate = tranlate; }
	void SetFovY(const float fovY) { fovY_ = fovY; }
	void SetAspectRatio(const float aspectRatio) { aspectRatio_ = aspectRatio; }
	void SetNearClip(const float nearClip) { nearClip_ = nearClip; }
	void SetFarClip(const float farClip) { farClip_ = farClip; }
	void SetBasePos(const Vector3& pos) { basePos_ = pos; }
	void SetBaseRot(const Vector3& rot) { baseRot_ = rot; }

	// getter
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix; }
	const Matrix4x4& GetViewMatrix() const { return viewMatrix; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix; }
	const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
	const Vector3& GetRotate() const { return transform_.rotate; }
	const Vector3& GetTranaslate() const { return transform_.translate; }

private:
	Transform transform_;
	Vector3 basePos_;
	Vector3 baseRot_;

	Matrix4x4 worldMatrix;
	Matrix4x4 viewMatrix;
	Matrix4x4 projectionMatrix;
	Matrix4x4 viewProjectionMatrix;

	float fovY_;
	float aspectRatio_;
	float nearClip_;
	float farClip_;

	struct ShakeInfo {
		Vector3 defaultTranslate_;
		Vector3 shakeOffset_ = { 0.0f,0.0f,0.0f };
		float shakeTimer_ = 0.0f;
		float shakeDuration_ = 0.0f;
		float shakeStrength_ = 0.0f;
		bool isShaking_ = false;
	};
	ShakeInfo shakeInfo_;
};