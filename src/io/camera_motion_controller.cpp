#include "io.hpp"
#include "imaging.hpp"
#include <functional>

using namespace tsw::io;
using namespace tsw::imaging;

CameraMotionController::CameraMotionController(FlirCamera& camera, OfficerLocator& officerLocator, DeviceSerialPort& commandPort)
{
    _camera = &camera;
    _officerLocator = &officerLocator;
    _commandPort = &commandPort;

    // These are the values for the firefly dl.
    HorizontalFov = 44.8;
    VerticalFov = 34.6;
}

void CameraMotionController::StartCameraMotionGuidance()
{
    if(!IsGuidingCameraMotion())
    {
        _isGuidingCameraMotion = true;
        _cameraLivefeedCallbackKey = _camera->RegisterLiveFeedCallback(bind(&CameraMotionController::OnLivefeedImageReceived, this, placeholders::_1));
    }
}

void CameraMotionController::StopCameraMotionGuidance()
{
    if(IsGuidingCameraMotion())
    {
        _camera->UnregisterLiveFeedCallback(_cameraLivefeedCallbackKey);
        _isGuidingCameraMotion = false;
    }
}

bool CameraMotionController::IsGuidingCameraMotion()
{
    return _isGuidingCameraMotion;
}

void CameraMotionController::OnLivefeedImageReceived(LiveFeedCallbackArgs args)
{
    // We don't always process every frame.
    // This will allow for a less jittery motion.
    if(args.imageIndex % (CameraFramesToSkip + 1))
    {
        return;
    }

    // Get the movement direction for this frame.
    OfficerDirection dir = _officerLocator->FindOfficer(args.image);

    // Based on the direction vector to move, we need to get the angles to rotate the motors.
    double horizontalRotate = dir.movement.x * HorizontalFov / 2;
    double verticalRotate = dir.movement.y * VerticalFov / 2;

    // Calculate the motor values for each of these.
    int horizontalMotor = AngleToMotorValue(horizontalRotate);
    int verticalMotor = AngleToMotorValue(verticalRotate);
    
    // The vertical bytes go after the horizontal.
    vector<uchar> bytes(6);
    for(int i = 0; i < 3; i++)
    {
        bytes[2 - i] = horizontalMotor >> (i * 8);
        bytes[5 - i] = verticalMotor >> (i * 8);
    }

    // Now we can send the command to the motor.
    // This is an asynchronous move since we don't really need to know when the motor has moved.
    _commandPort->WriteToDevice(Motors, bytes);

    // Wait for the acknowledge (not the same as a synch response).
    _commandPort->ReadFromDevice(Motors);
}

int CameraMotionController::AngleToMotorValue(double angle)
{
    // Get a value between -1 and 1 for how close this guy is to +-360 degrees.
    double angleProp = angle / 360;

    // Now we can multiply this by the highest 3 byte positive value that has a negative value in 3 bytes (2^23 - 1).
    return (int)(angleProp * 0x7fffff);
}