#include "Camera.h"
#include "Engine/Base/WinApp/WinApp.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif // DEBUG

Camera::Camera()
	: transform_({
		  {1.0f, 1.0f, 1.0f},
		  {0.0f, 0.0f, 0.0f},
		  {0.0f, 0.0f, 0.0f}
		}),
	fovY_(0.45f), aspectRatio_(float(WinApp::kWindowWidth_) / float(WinApp::kWindowHeight_)), nearClip_(0.1f), farClip_(100.0f),
	worldMatrix(MyMath::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate)), viewMatrix(MyMath::Inverse4x4(worldMatrix)),
	projectionMatrix(MyMath::MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_)), viewProjectionMatrix(MyMath::Multiply(viewMatrix, projectionMatrix)) {
}

void Camera::Init() {}

void Camera::Update() {

	CameraShake();

	Vector3 finalPos = transform_.translate + shakeInfo_.shakeOffset_;

	Matrix4x4 scaleMat = MyMath::MakeScaleMatrix(transform_.scale);
	Matrix4x4 rotMat = MyMath::MakeRotateMatrix(transform_.rotate);
	Matrix4x4 transMat = MyMath::MakeTranslateMatrix(finalPos);

	worldMatrix = MyMath::Multiply(scaleMat, MyMath::Multiply(rotMat, transMat));
	viewMatrix = MyMath::Inverse4x4(worldMatrix);
	projectionMatrix = MyMath::MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_);
	viewProjectionMatrix = MyMath::Multiply(viewMatrix, projectionMatrix);
}


void Camera::ImGuiDebug() {

#ifdef USE_IMGUI

	ImGui::Begin("Camera");

	ImGui::SliderAngle("rotateX", &transform_.rotate.x);
	ImGui::SliderAngle("rotateY", &transform_.rotate.y);
	ImGui::SliderAngle("rotateZ", &transform_.rotate.z);
	ImGui::DragFloat3("transform", &transform_.translate.x, 0.01f);
	ImGui::Text("Base Pos: (%.2f, %.2f, %.2f)", basePos_.x, basePos_.y, basePos_.z);
	ImGui::Text("Shake Offset: (%.2f, %.2f, %.2f)", shakeInfo_.shakeOffset_.x, shakeInfo_.shakeOffset_.y, shakeInfo_.shakeOffset_.z);
	ImGui::Checkbox("Is Shaking", &shakeInfo_.isShaking_);
	ImGui::DragFloat("Shake Strength", &shakeInfo_.shakeStrength_, 0.01f);
	ImGui::DragFloat("Shake Duration", &shakeInfo_.shakeDuration_, 0.01f);

	ImGui::End();

#endif
}

void Camera::SetEye(const Vector3& eye) { transform_.translate = eye; }

void Camera::SetTarget(const Vector3& target) {
	Vector3 dir = { target.x - transform_.translate.x, target.y - transform_.translate.y, target.z - transform_.translate.z };

	// 
	dir = MyMath::Normalize(dir);

	// 
	// float pitch = std::asin(-dir.y);
	float yaw = std::atan2(dir.x, dir.z);

	transform_.rotate = { 0.0f, yaw, 0.0f };
}

void Camera::StartShake(CameraShakeType type) {

	// 
	switch (type) {
	case CameraShakeType::Small:
		shakeInfo_.shakeDuration_ = 0.3f;
		shakeInfo_.shakeStrength_ = 0.1f;
		break;
	case CameraShakeType::Medium:
		shakeInfo_.shakeDuration_ = 0.5f;
		shakeInfo_.shakeStrength_ = 0.2f;
		break;
	case CameraShakeType::Large:
		shakeInfo_.shakeDuration_ = 0.8f;
		shakeInfo_.shakeStrength_ = 0.4f;
		break;
	}
	shakeInfo_.shakeTimer_ = 0.0f;
	shakeInfo_.isShaking_ = true;
}

void Camera::CameraShake() {

	// 
	if (shakeInfo_.isShaking_) {
		shakeInfo_.shakeTimer_ += 1.0f / 60.0f;

		float x = (rand() / (float)RAND_MAX - 0.5f) * 2.0f * shakeInfo_.shakeStrength_;
		float y = (rand() / (float)RAND_MAX - 0.5f) * 2.0f * shakeInfo_.shakeStrength_;
		float z = (rand() / (float)RAND_MAX - 0.5f) * 2.0f * shakeInfo_.shakeStrength_;

		shakeInfo_.shakeOffset_ = { x, y, z };

		if (shakeInfo_.shakeTimer_ >= shakeInfo_.shakeDuration_) {
			shakeInfo_.isShaking_ = false;
			shakeInfo_.shakeOffset_ = { 0.0f, 0.0f, 0.0f };
		}
	}
}