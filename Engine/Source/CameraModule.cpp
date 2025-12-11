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
}

CameraModule::~CameraModule()
{
}

bool CameraModule::init()
{
    view = SimpleMath::Matrix::CreateLookAt(SimpleMath::Vector3(position), SimpleMath::Vector3(target), SimpleMath::Vector3::Up);
    return true;
  
}

void CameraModule::update()
{
    // Local directions
    forward = (target - position);
    forward.Normalize();
    right = forward.Cross(up);
    right.Normalize();

    float elapsedSec = app->getElapsedMilis() * 0.001f;

    Keyboard::State kb = Keyboard::Get().GetState();

    if (kb.IsKeyDown(Keyboard::Keys::W)) position += forward * speed * elapsedSec;
    if (kb.IsKeyDown(Keyboard::Keys::S)) position -= forward * speed * elapsedSec;
    if (kb.IsKeyDown(Keyboard::Keys::A)) position -= right * speed * elapsedSec;
    if (kb.IsKeyDown(Keyboard::Keys::D)) position += right * speed * elapsedSec;
    if (kb.IsKeyDown(Keyboard::Keys::Q)) position += up * speed * elapsedSec;
    if (kb.IsKeyDown(Keyboard::Keys::E)) position -= up * speed * elapsedSec;
    if (kb.IsKeyDown(Keyboard::Keys::F)) position = SimpleMath::Vector3(0.0f, 3.0f, 10.0f);


    target = position + forward;

    view = Matrix::CreateLookAt(position, target, up);
}

