#pragma once

#include "Engine/lib/Math/MyMath.h"
#include "struct.h"

class Camera {

public:
	Camera();
	~Camera() = default;

	void Init();

	void Update();

	void ImGuiDebug();

public:
	// setter
	void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
	void SetTranslate(const Vector3& tranlate) { transform_.translate = tranlate; }
	void SetFovY(const float fovY) { fovY_ = fovY; }
	void SetAspectRatio(const float aspectRatio) { aspectRatio_ = aspectRatio; }
	void SetNearClip(const float nearClip) { nearClip_ = nearClip; }
	void SetFarClip(const float farClip) { farClip_ = farClip; }
	
	// getter
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix; }
	const Matrix4x4& GetViewMatrix() const { return viewMatrix; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix; }
	const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
	const Vector3& GetRotate() const { return transform_.rotate; }
	const Vector3& GetTranaslate() const { return transform_.translate; }

private:
	Transform transform_;

	Matrix4x4 worldMatrix;
	Matrix4x4 viewMatrix;
	Matrix4x4 projectionMatrix;
	Matrix4x4 viewProjectionMatrix;

	float fovY_;
	float aspectRatio_;
	float nearClip_;
	float farClip_;
};