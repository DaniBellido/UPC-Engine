#include "Globals.h"
#include "CameraModule.h"
#include "Application.h"

#include "Mouse.h"
#include "Keyboard.h"
#include "GamePad.h"

CameraModule::CameraModule() 
{
    position = SimpleMath::Vector3(0.0f, 3.0f, 10.0f);
    target = SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
    rotation = Quaternion(0.00f, 1.00f, -0.10f, -0.02f);
}

CameraModule::~CameraModule()
{
}

bool CameraModule::init()
{
    yaw = 0.0f;
    pitch = 0.0f; 
    forward = Vector3(0, 0, 1); 
    up = Vector3::UnitY;

    view = SimpleMath::Matrix::CreateLookAt(SimpleMath::Vector3(position), SimpleMath::Vector3(target), SimpleMath::Vector3::Up);

    return true;
  
}

void CameraModule::update()
{
    float dt = app->getElapsedMilis() * 0.001f;

    auto& mouse = Mouse::Get();
    Mouse::State ms = mouse.GetState();

    static bool rotating = false;
    static int lastX = 0, lastY = 0;

    if (ms.rightButton && !rotating) { rotating = true; lastX = ms.x; lastY = ms.y; }
    else if (!ms.rightButton && rotating) { rotating = false; }

    if (rotating) {
        int dx = ms.x - lastX, dy = ms.y - lastY;
        lastX = ms.x; lastY = ms.y;
        yaw += dx * 0.002f;
        pitch -= dy * 0.002f;
        if (pitch > 1.5f) pitch = 1.5f;
        if (pitch < -1.5f) pitch = -1.5f;
    }

    forward.x = cosf(pitch) * sinf(yaw);
    forward.y = sinf(pitch);
    forward.z = cosf(pitch) * cosf(yaw);
    forward.Normalize();

    right = Vector3(0, 1, 0).Cross(forward);
    right.Normalize();

    up = forward.Cross(right);
    up.Normalize();

    Matrix rotMatrix = Matrix::CreateLookAt(Vector3::Zero, forward, up);
    rotation = Quaternion::CreateFromRotationMatrix(rotMatrix);

    // MOVIMIENTO
    Keyboard::State kb = Keyboard::Get().GetState();
    if (kb.IsKeyDown(Keyboard::Keys::W)) position += forward * speed * dt;
    if (kb.IsKeyDown(Keyboard::Keys::S)) position -= forward * speed * dt;
    if (kb.IsKeyDown(Keyboard::Keys::D)) position -= right * speed * dt;
    if (kb.IsKeyDown(Keyboard::Keys::A)) position += right * speed * dt;
    if (kb.IsKeyDown(Keyboard::Keys::Q)) position += up * speed * dt;
    if (kb.IsKeyDown(Keyboard::Keys::E)) position -= up * speed * dt;
    if (kb.IsKeyDown(Keyboard::Keys::F)) {
        position = Vector3(0, 3, 10);
        yaw = 0; pitch = 0;
        forward = Vector3(0, 0, 1);
        right = Vector3(1, 0, 0);
        up = Vector3(0, 1, 0);
        rotation = Quaternion(0.00f, 1.00f, -0.10f, -0.02f);
    }

    target = position + forward;
    view = Matrix::CreateLookAt(position, target, up);
}

