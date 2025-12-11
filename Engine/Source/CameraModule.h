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

public:
	CameraModule();
	~CameraModule();

	bool init() override;
	void update() override;



	float setSpeed(float _speed) { return speed = _speed; }

	const Matrix& getView() const { return view; }
	const Quaternion& getRot() const { return rotation; }
	const Vector3& getPos() const { return position; }
	const float& getSpeed() const { return speed; }


	/*Add methods to manipulate it :
	SetFOV() … should set the horizontal FOV keeping the aspect ratio
		SetAspectRatio() … should change the vertical FOV to meet the new aspect ratio.
		SetPlaneDistances() / Position() / Orientation() / LookAt(x, y, z)
		GetProjectionMatrix()
		GetViewMatrix()…*/

};

