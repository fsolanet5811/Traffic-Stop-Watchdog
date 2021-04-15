#include "io.hpp"
#include "imaging.hpp"
#include <functional>

using namespace tsw::io;
using namespace tsw::imaging;

CameraMotionController::CameraMotionController(MotorController& motorController)
{
    _motorController = &motorController;
    HomeAngles.x = 0;
    HomeAngles.y = 0;
    AngleXBounds.min = 0;
    AngleXBounds.max = 359;

    // The default speed will be half way.
    MotorSpeeds.x = 127;
    MotorSpeeds.x = 127;

    // These are the values for the firefly dl.
    HorizontalFov = 44.8;
    VerticalFov = 34.6;

    // By default, we did not find an officer.
    ResetSearchState();
    _isGuidanceInitialized = false;
}

void CameraMotionController::ResetSearchState()
{
    _lastSeen.foundOfficer = false;
    _searchState = NotSearching;
}

bool CameraMotionController::IsGuidanceInitialized()
{
    return _isGuidanceInitialized;
}

void CameraMotionController::InitializeGuidance()
{
    if(!IsGuidanceInitialized())
    {
        // Activate the motors.
        _motorController->Activate();

        // Set the speeds.
        _motorController->SetSpeeds(MotorSpeeds);

        _isGuidanceInitialized = true;
    }
}

void CameraMotionController::UninitializeGuidance()
{
    if(IsGuidanceInitialized())
    {
        // Deactivate the motors.
        _motorController->Deactivate();

        _isGuidanceInitialized = false;
        ResetSearchState();
    }
}

void CameraMotionController::GuideCameraTo(OfficerDirection location)
{
    if(location.foundOfficer)
    {
        // Add a reference to where we last saw the officer.
        _lastSeen = location;

        // Reset the officer search state.
        _searchState = NotSearching;

        // We don't always have to move if we found the officer.
        if(location.shouldMove)
        {
            // Based on the direction vector to move, we need to get the angles to rotate the motors.
            // Positive angles point to the left/down, so we gotta negate these guys.
            double horizontalRotate = location.movement.x * HorizontalFov / 2;
            double verticalRotate = -1 * location.movement.y * VerticalFov / 2;

            _motorController->SendAsyncRelativeMoveCommand(horizontalRotate, verticalRotate);
        }
        else
        {
            Log("Officer found, halting motors", Movements | Officers);
            _motorController->SendAsyncRelativeMoveCommand(0, 0);
        }       
    }
    else
    {
        // We need to find the officer.
        OfficerSearch();
    }
    
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
            if(_motorController->TryReadMessage(&temp))
            {
                // We made it, at this point the officer probably is not there, so just go home.
                _searchState = Circling;
                MoveToMin();
            }
            break;

        case Circling:
            // Keep circling.
            Circle();
            break;
    }
}

void CameraMotionController::CalibrateFOV(int frameWidth, int frameHeight)
{
    // We know the FOV for 1440 x 1080, and the relationship is linear!
    HorizontalFov = 44.8 * frameWidth / 1440.0;
    VerticalFov = 34.6 * frameHeight / 1080.0;
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
        _motorController->SendSyncAbsoluteMoveCommand(horizontalAngle, verticalAngle);

        // Reset where we last saw the officer.
        _lastSeen.foundOfficer = false;
    }
    else
    {
        // We have never seen the officer. Just start circling.
        _searchState = Circling;
        MoveToMin();
    }
}

void CameraMotionController::GoToHome()
{
    Log("Going to home position", Movements);
    _motorController->SendSyncAbsoluteMoveCommand(HomeAngles.x, HomeAngles.y);
}

void CameraMotionController::Circle()
{
    DeviceMessage response;
    if(_motorController->TryReadMessage(&response))
    {
        // They made it to the end. Now go to the other end.
        if(_movingTowardsMin)
        {
            MoveToMax();
        }
        else
        {
            MoveToMin();
        }
    }
}

void CameraMotionController::MoveToMin()
{
    _movingTowardsMin = true;
    _motorController->SendSyncAbsoluteMoveCommand(AngleXBounds.min, HomeAngles.y);
}

void CameraMotionController::MoveToMax()
{
    _movingTowardsMin = false;
    _motorController->SendSyncAbsoluteMoveCommand(AngleXBounds.max, HomeAngles.y);
}
