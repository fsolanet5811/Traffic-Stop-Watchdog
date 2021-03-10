#include "io.hpp"
#include "imaging.hpp"
#include <functional>

using namespace tsw::io;
using namespace tsw::imaging;

CameraMotionController::CameraMotionController(FlirCamera& camera, OfficerLocator& officerLocator, MotorController& motorController)
{
    _camera = &camera;
    _officerLocator = &officerLocator;
    _motorController = &motorController;
    CameraFramesToSkip = 0;
    HomeAngles.x = 0;
    HomeAngles.y = 0;

    // These are the values for the firefly dl.
    HorizontalFov = 44.8;
    VerticalFov = 34.6;

    // By deafult, we did not find an officer.
    ResetSearchState();
    _isGuidingCameraMotion = false;
}

void CameraMotionController::ResetSearchState()
{
    _lastSeen.foundOfficer = false;
    _searchState = NotSearching;
}

void CameraMotionController::StartCameraMotionGuidance()
{
    if(!IsGuidingCameraMotion())
    {
        // Activate the motors.
        _motorController->Activate();

        _isGuidingCameraMotion = true;
        _cameraLivefeedCallbackKey = _camera->RegisterLiveFeedCallback(bind(&CameraMotionController::OnLivefeedImageReceived, this, placeholders::_1));
    }
}

void CameraMotionController::StopCameraMotionGuidance()
{
    if(IsGuidingCameraMotion())
    {
        _camera->UnregisterLiveFeedCallback(_cameraLivefeedCallbackKey);

        // Deactivate the motors.
        _motorController->Deactivate();

        _isGuidingCameraMotion = false;
        ResetSearchState();
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

            _motorController->SendAsyncRelativeMoveCommand(horizontalRotate, verticalRotate);
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
                _searchState = MovingToHomePosition;
                GoToHome();
            }
            break;

        case MovingToHomePosition:
            // Check if we are there yet.
            if(_motorController->TryReadMessage(&temp))
            {
                // We made it, start circling.
                _searchState = Circling;
                _motorController->SendAsyncRelativeMoveCommand(20, 0);
            }
            break;

        case Circling:
            // Keep circling.
            _motorController->SendAsyncRelativeMoveCommand(20, 0);
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
        _motorController->SendSyncAbsoluteMoveCommand(horizontalAngle, verticalAngle);
    }
    else
    {
        // We have never seen the officer. Just go to home and start circling.
        _searchState = MovingToHomePosition;
        GoToHome();
    }
}

void CameraMotionController::GoToHome()
{
    Log("Going to home position", Movements);
    _motorController->SendSyncAbsoluteMoveCommand(HomeAngles.x, HomeAngles.y);
}