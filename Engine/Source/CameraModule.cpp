#include "Globals.h"
#include "CameraModule.h"

#include "Application.h"
#include "D3D12Module.h"

#include "Mouse.h"
#include "Keyboard.h"
#include "GamePad.h"

void CameraModule::FocusAt(const Vector3& point)
{
    target = point;

    // Dirección correcta hacia el objeto
    forward = target - position;
    forward.Normalize();

    // Reconstruir yaw / pitch desde forward
    yaw = atan2f(forward.x, -forward.z);
    pitch = asinf(forward.y);

    // Orthonormal basis
    right = Vector3::UnitY.Cross(forward);
    right.Normalize();
    up = forward.Cross(right);
    up.Normalize();
}

CameraModule::CameraModule()
{
    // -------------------------------------------------------------------------------
    // INITIAL CAMERA SETUP
    // -------------------------------------------------------------------------------
    position = SimpleMath::Vector3(0.0f, 3.0f, 10.0f);  // Camera starts at (0,3,10)
    target = SimpleMath::Vector3(0.0f, 0.0f, 0.0f);     // Looking at origin (0,0,0)

    // Euler angles
    yaw = 0.0f;                                         //yaw=0 (look -Z)
    pitch = 0.0f;                                       // pitch=0 (level horizon)

    rotation = Quaternion::Identity;                    // Default quaternion: no rotation
}

CameraModule::~CameraModule()
{
}

bool CameraModule::init()
{
    // -------------------------------------------------------------------------------
    // INITIALIZE CAMERA VECTORS
    // -------------------------------------------------------------------------------
    forward = Vector3(0, 0, -1);                        // Look direction: towards origin (-Z)
    up = Vector3::UnitY;                                // World up vector

    FocusAt(Vector3::Zero);

    D3D12Module* d3d12 = app->getD3D12();
    aspect = float(d3d12->getWindowWidth()) / float(d3d12->getWindowHeight());

    // Create initial view matrix using LookAt
    view = SimpleMath::Matrix::CreateLookAt(SimpleMath::Vector3(position), SimpleMath::Vector3(target), SimpleMath::Vector3::Up);

    return true;
  
}

void CameraModule::update()
{
    float dt = app->getElapsedMilis() * 0.001f;

    // ------------------------------------------------------------------------------
    // MOUSE ROTATION INPUT
    // ------------------------------------------------------------------------------
    // Detect right mouse button for camera rotation (FPS style)
    auto& mouse = Mouse::Get();
    Mouse::State ms = mouse.GetState();
    static bool rotating = false;
    static int lastX = 0, lastY = 0;

    // Start/stop rotation mode with right mouse button
    if (ms.rightButton && !rotating) { rotating = true; lastX = ms.x; lastY = ms.y; }
    else if (!ms.rightButton && rotating) { rotating = false; }

    if (rotating)
    {
        // Calculate mouse delta and update Euler angles
        int dx = ms.x - lastX, dy = ms.y - lastY;
        lastX = ms.x; lastY = ms.y;
        yaw += dx * 0.002f;               // Horizontal rotation (left/right)
        pitch -= dy * 0.002f;             // Vertical rotation (up/down)

        // Clamp pitch to avoid flipping (gimbal lock prevention)
        if (pitch > 1.5f) pitch = 1.5f;
        if (pitch < -1.5f) pitch = -1.5f;
    }

    // ------------------------------------------------------------
    // MOUSE WHEEL ZOOM
    // ------------------------------------------------------------
    if (ms.scrollWheelValue != 0)
    {
        // The scrollWheelValue is cumulative, so we save the delta.
        static int lastWheel = 0;
        int wheelDelta = ms.scrollWheelValue - lastWheel;
        lastWheel = ms.scrollWheelValue;

        float zoomSpeed = 5.0f;
        float zoomAmount = wheelDelta * zoomSpeed * dt;

        // New position
        Vector3 newPos = position + forward * zoomAmount;

        // Current distance to target
        float dist = (position - target).Length();

        // Distance threshold
        const float minDist = 1.0f;
        const float maxDist = 200.0f;

        float newDist = (newPos - target).Length();

        // Zoom in ONLY if it is within the limits
        if (newDist > minDist && newDist < maxDist)
        {
            position = newPos;
        }
    }

    // ----------------------------------------------------------------------------
    // FORWARD VECTOR CALCULATION (Yaw/Pitch -> Direction)
    // ----------------------------------------------------------------------------
    // Convert Euler angles to forward direction vector (FPS convention)
    forward.x = sinf(yaw) * cosf(pitch);
    forward.y = sinf(pitch);
    forward.z = cosf(yaw) * cosf(pitch);
    forward.z = -forward.z;  
    forward.Normalize();

    // ---------------------------------------------------------------------------
    // ORTHONORMAL BASIS CALCULATION
    // ---------------------------------------------------------------------------
    // Compute right and up vectors to form complete camera basis
    right = Vector3::UnitY.Cross(forward);
    right.Normalize();
    up = forward.Cross(right);
    up.Normalize();

    // --------------------------------------------------------------------------
    // QUATERNION ROTATION MATRIX
    // --------------------------------------------------------------------------
    Matrix rotMatrix = Matrix::CreateLookAt(Vector3::Zero, forward, up);
    rotation = Quaternion::CreateFromRotationMatrix(rotMatrix);


    // -------------------------------------------------------------------------
    // SET SPEED (SHIFT)
    // -------------------------------------------------------------------------
    Keyboard::State kb = Keyboard::Get().GetState();
    if (kb.IsKeyDown(Keyboard::Keys::LeftShift))
        speed = 10.0f;
    else
        speed = 5.0f;

    // --------------------------------------------------------------------------
    // KEYBOARD MOVEMENT (WASDQE)
    // --------------------------------------------------------------------------
    // Move camera in local space using forward/right/up vectors
    if (kb.IsKeyDown(Keyboard::Keys::W)) position += forward * speed * dt;
    if (kb.IsKeyDown(Keyboard::Keys::S)) position -= forward * speed * dt;
    if (kb.IsKeyDown(Keyboard::Keys::A)) position += right * speed * dt;
    if (kb.IsKeyDown(Keyboard::Keys::D)) position -= right * speed * dt;
    if (kb.IsKeyDown(Keyboard::Keys::Q)) position -= up * speed * dt;
    if (kb.IsKeyDown(Keyboard::Keys::E)) position += up * speed * dt;

    // -------------------------------------------------------------------------
    // RESET FOCUS (F KEY)
    // -------------------------------------------------------------------------
    // Reset to initial position and orientation
    if (kb.IsKeyDown(Keyboard::Keys::F)) 
    {
        position = Vector3(0, 3, 10);
        FocusAt(Vector3::Zero);
    }

    // ------------------------------------------------------------------------
    // CAMERA ORBIT AROUND (0,0,0) (Alt + Left Click)
    // ------------------------------------------------------------------------
    static bool orbiting = false;
    static int  orbitLastX = 0;
    static int  orbitLastY = 0;

    // Orbit center of the 3D Space
    Vector3 orbitCenter = Vector3::Zero;   

    bool altDown = kb.IsKeyDown(Keyboard::Keys::LeftAlt) || kb.IsKeyDown(Keyboard::Keys::RightAlt);

    if (altDown && ms.leftButton && !orbiting)
    {
        orbiting = true;
        orbitLastX = ms.x;
        orbitLastY = ms.y;

        // Target looks at the center
        target = orbitCenter;
    }
    else if ((!altDown || !ms.leftButton) && orbiting)
    {
        orbiting = false;
    }

    if (orbiting)
    {
        int dx = ms.x - orbitLastX;
        int dy = ms.y - orbitLastY;
        orbitLastX = ms.x;
        orbitLastY = ms.y;

        float orbitSpeed = 0.002f;
        yaw += dx * orbitSpeed;
        pitch -= dy * orbitSpeed;

        // Vertical clamp
        if (pitch > 1.5f)  pitch = 1.5f;
        if (pitch < -1.5f) pitch = -1.5f;

        // Radius to center
        float radius = (position - orbitCenter).Length();
        if (radius < 0.001f) radius = 0.001f;

        Vector3 dir;
        dir.x = sinf(yaw) * cosf(pitch);
        dir.y = sinf(pitch);
        dir.z = cosf(yaw) * cosf(pitch);
        dir.z = -dir.z;
        dir.Normalize();

        // Camera in orbit looking at the center
        position = orbitCenter - dir * radius;
        forward = (orbitCenter - position);
        forward.Normalize();
        right = Vector3::UnitY.Cross(forward);
        right.Normalize();
        up = forward.Cross(right);
        up.Normalize();

        // if orbiting, target is always the center
        target = orbitCenter;
    }

    // ------------------------------------------------------------------------
    // FINAL VIEW MATRIX
    // ------------------------------------------------------------------------
    // Update target and create final view matrix
    target = position + forward;
    view = Matrix::CreateLookAt(position, target, up);

   
}

