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
    CameraFramesToSkip = 0;
    HomeAngles.x = 0;
    HomeAngles.y = 0;

    // These are the values for the firefly dl.
    HorizontalFov = 44.8;
    VerticalFov = 34.6;

    // By deafult, we did not find an officer.
    _lastSeen.foundOfficer = false;
    _searchState = NotSearching;
    _isGuidingCameraMotion = false;

    // Set the default angles/steps.
    MinAngle = -360;
    MaxAngle = 360;
    MinStep = -1000;
    MaxStep = 1000;
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

void CameraMotionController::SendMoveCommand(uchar specifierByte, double horizontal, double vertical, string moveName)
{
    // Calculate the motor values for each of these.
    int horizontalMotor = AngleToMotorValue(horizontal);
    int verticalMotor = AngleToMotorValue(vertical);
    
    // The vertical bytes go after the horizontal.
    vector<uchar> bytes(7);
    bytes[0] = specifierByte;
    for(int i = 0; i < 3; i++)
    {
        bytes[3 - i] = (horizontalMotor >> (i * 8)) & 0xff;
        bytes[6 - i] = (verticalMotor >> (i * 8)) & 0xff;
    }

    // Now we can send the command to the motor.
    // This is an asynchronous move since we don't really need to know when the motor has moved.
    Log("MOVE " + moveName + "\tH:  " + to_string(horizontal) + "\tV:  " + to_string(vertical), Movements);
    _commandPort->WriteToDevice(Motors, bytes);

    // Wait for the acknowledge (not the same as a synch response).
    // It is possible that the read response is not an ack but a success/failure from a previous move.
    while(true)
    {
        Log("Reading move command acknowledge", Acknowledge);
        DeviceMessage message = _commandPort->ReadFromDevice(Motors);

        // Acknowledgements will be four 1's in the lsbs.
        uchar lsbs = message.bytes[0] & 0x0f;
        if(lsbs == 0x0f)
        {
            // This is the acknowledgement.
            Log("Move command acknowledge received", Acknowledge);
            return;
        }

        if(lsbs == 0x02)
        {
            // There was a fault in the motors' last movement.
            Log("Motors returned FAULT!", Error);
        }

        // The byte was a success byte, just read another one.
        Log("MOve command success response read", Acknowledge);
    }
    
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

    if(dir.foundOfficer)
    {
        // Reset the officer search state.
        _searchState = NotSearching;

        // We don't always have to move if we found the officer.
        if(dir.shouldMove)
        {
            // Based on the direction vector to move, we need to get the angles to rotate the motors.
            double horizontalRotate = dir.movement.x * HorizontalFov / 2;
            double verticalRotate = dir.movement.y * VerticalFov / 2;

            SendAsyncRelativeMoveCommand(horizontalRotate, verticalRotate);
        }
        else
        {
            Log("Officer found, no movements necessary", Movements | Officers);
        }       
    }
    else
    {
        // We need to find the officer.
        OfficerSearch();
    }
    
}

int CameraMotionController::AngleToMotorValue(double angle)
{
    // Get a value between 0 and 1 for how close to the max angle our given angle is.
    // 1 represents the max angle and 0 represents the min angle.
    // Technically if our value is outside the bounds, this number will be outside [0, 1]
    double angleProp = (angle - MinAngle) / (MaxAngle - MinAngle);

    // Now we can multiply this by the highest 3 byte positive value that has a negative value in 3 bytes (2^23 - 1).
    return (int)(MinStep + angleProp * (MaxStep - MinStep));
}

void CameraMotionController::OfficerSearch()
{
    DeviceMessage temp;
    switch(_searchState)
    {
        case NotSearching:
            // We will check the last seen position.
            CheckLastSeen();
            break;

        case CheckingLastSeen:
            // Check if we are there yet.
            if(_commandPort->TryReadFromDevice(Motors, &temp))
            {
                // We made it, at this point the officer probably is not ther, so just go home.
                _searchState = MovingToHomePosition;
                GoToHome();
            }
            break;

        case MovingToHomePosition:
            // Check if we are there yet.
            if(_commandPort->TryReadFromDevice(Motors, &temp))
            {
                // We made it, start circling.
                _searchState = Circling;
                SendAsyncRelativeMoveCommand(20, 0);
            }
            break;

        case Circling:
            // Keep circling.
            SendAsyncRelativeMoveCommand(20, 0);
            break;
    }
}

void CameraMotionController::CheckLastSeen()
{
    _searchState = CheckingLastSeen;

    // See if we actually have a last seen position.
    if(_lastSeen.foundOfficer)
    {
        Log("Checking last seen officer direction for officer", Officers);

        // Assume that the officer went in that direction.
        double horizontalAngle = _lastSeen.movement.x * HorizontalFov;
        double verticalAngle = _lastSeen.movement.y * VerticalFov;

        // Move so that the center of the frame is on the officer's predicted location.
        SendSyncAbsoluteMoveCommand(horizontalAngle, verticalAngle);
    }
    else
    {
        // We have never seen the officer. Just go to home and start circling.
        _searchState = MovingToHomePosition;
        GoToHome();
    }
}

void CameraMotionController::SendAsyncRelativeMoveCommand(double horizontal, double vertical)
{
    SendMoveCommand(0xe6, horizontal, vertical, "Async Rel");
}

void CameraMotionController::SendSyncRelativeMoveCommand(double horizontal, double vertical)
{
    SendMoveCommand(0xe5, horizontal, vertical, "Sync Rel");
}

void CameraMotionController::SendAsyncAbsoluteMoveCommand(double horizontal, double vertical)
{
    SendMoveCommand(0xe8, horizontal, vertical, "Async Abs");
}

void CameraMotionController::SendSyncAbsoluteMoveCommand(double horizontal, double vertical)
{
    SendMoveCommand(0xe7, horizontal, vertical, "Sync Abs");
}

void CameraMotionController::GoToHome()
{
    Log("Going to home position", Movements);
    SendSyncAbsoluteMoveCommand(HomeAngles.x, HomeAngles.y);
}