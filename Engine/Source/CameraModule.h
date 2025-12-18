#pragma once
#include "Module.h"
class CameraModule : public Module
{
	Quaternion rotation;
	Vector3 position;
	Vector3 target;
	Matrix view;

	Vector3 forward;    // camera direction
	Vector3 right;      // perpendicular direction
	Vector3 up = Vector3::Up; // Camera always up
	float speed = 5.0f; 
	float yaw = 0.0f;
	float pitch = 0.0f;
	float mouseSensitivity = 0.002f;

	float aspect;

	float fov = XM_PIDIV4;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;

public:
	CameraModule();
	~CameraModule();

	bool init() override;
	void update() override;

	void FocusAt(const Vector3& point);
	float setSpeed(float _speed) { return speed = _speed; }
	float SetFov(float newFov) { return fov = newFov; }
	float SetNearPlane(float nearP) { return nearPlane = nearP; }
	float SetFarPlane(float farP) { return farPlane = farP; }

	const Matrix& getView() const { return view; }
	SimpleMath::Matrix GetProjection(float aspect) const { return SimpleMath::Matrix::CreatePerspectiveFieldOfView(fov, aspect, nearPlane, farPlane); }
	const Quaternion& getRot() const { return rotation; }
	const Vector3& getPos() const { return position; }
	const float& getSpeed() const { return speed; }
	const float getAspect() const { return aspect; }
	float GetFov() const { return fov; }
	float GetNearPlane() const { return nearPlane; }
	float GetFarPlane() const { return farPlane; }
};

