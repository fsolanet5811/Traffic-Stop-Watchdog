#include "io.hpp"
#include "utilities.hpp"

using namespace tsw::io;
using namespace tsw::utilities;

MotorController::MotorController(DeviceSerialPort& commandPort, MotorConfig panConfig, MotorConfig tiltConfig)
{
    _commandPort = &commandPort;
    PanConfig = panConfig;
    TiltConfig = tiltConfig;
    _headlightsState = 0;
}

void MotorController::SendAsyncRelativeMoveCommand(double horizontal, double vertical)
{
    SendMoveCommand(RelativeMoveAsynchronous, horizontal, vertical, "Async Rel");
}

void MotorController::SendSyncRelativeMoveCommand(double horizontal, double vertical)
{
    SendMoveCommand(RelativeMoveSynchronous, horizontal, vertical, "Sync Rel");
}

void MotorController::SendAsyncAbsoluteMoveCommand(double horizontal, double vertical)
{
    SendMoveCommand(AbsoluteMoveAsynchronous, horizontal, vertical, "Async Abs");
}

void MotorController::SendSyncAbsoluteMoveCommand(double horizontal, double vertical)
{
    SendMoveCommand(AbsoluteMoveSynchronous, horizontal, vertical, "Sync Abs");
}

void MotorController::SendMoveCommand(CommandAction moveType, double horizontal, double vertical, string moveName)
{
    // Calculate the motor values for each of these.
    int horizontalMotor = AngleToMotorValue(horizontal, PanConfig);
    int verticalMotor = AngleToMotorValue(vertical, TiltConfig);
    
    // The vertical bytes go after the horizontal.
    vector<unsigned char> bytes(6);
    for(int i = 0; i < 3; i++)
    {
        bytes[2 - i] = (horizontalMotor >> (i * 8)) & 0xff;
        bytes[5 - i] = (verticalMotor >> (i * 8)) & 0xff;
    }

    // Now we can send the command to the motor.
    // This is an asynchronous move since we don't really need to know when the motor has moved.
    // Give them the steps as well.
    Log("MOVE " + moveName + "\tH:  " + to_string(horizontal) + "  (" + to_string(horizontalMotor) + ")\tV:  " + to_string(vertical) + "  (" + to_string(verticalMotor) + ")", Movements);
    _commandPort->WriteToDevice(Motors, moveType, bytes);

    // Wait for the acknowledge (not the same as a synch response).
    // It is possible that the read response is not an ack but a success/failure from a previous move.
    ReadAcknowledge();
}

unsigned char MotorController::GetHeadlightsState()
{
    return _headlightsState;
}

void MotorController::SetHeadlightsState(unsigned char state)
{
    Log("Setting headlights state to " + to_string(state), LED);

    // We do not have to bother sending it the command if the state won't change.
    if(GetHeadlightsState() != state)
    {
        _commandPort->WriteToDevice(Motors, Headlights, state);
        ReadAcknowledge();
        _headlightsState = state;
        Log("Headlights state set", LED);
    }
    else
    {
        Log("Headlights already in state " + to_string(state), LED);
    }
}

void MotorController::Activate()
{
    Log("Activating motors", Movements);
    _commandPort->WriteToDevice(Motors, tsw::io::Activate);

    ReadAcknowledge();

	// The motors will send us a byte when it is calibrated.
	// We gotta wait for this before starting anything else.
    ReadSuccess();

    Log("Motors activated", Movements);
}

void MotorController::Deactivate()
{
    Log("Deactivating motors", Movements);
    _commandPort->WriteToDevice(Motors, tsw::io::Deactivate);
    
    ReadAcknowledge();
    
    Log("Motors Deactivated", Motors);
}

bool MotorController::TryReadMessage(DeviceMessage* message)
{
    return _commandPort->TryReadFromDevice(Motors, message);
}

void MotorController::SetSpeeds(ByteVector2 speeds)
{
    // To keep it consistent, we will send the horizontal speed first.
    vector<unsigned char> data;
    data.push_back(speeds.x);
    data.push_back(speeds.y);

    // Now send the command.
    Log("Settings motor speeds to (" + to_string((int)speeds.x) + ", " + to_string((int)speeds.y) + ")", Movements);
    _commandPort->WriteToDevice(Motors, tsw::io::SetSpeeds, data);

    // They will send an ack when they get it.
    ReadAcknowledge();

    Log("Motor speeds set", Movements);
}

void MotorController::ReadAcknowledge()
{
    Log("Waiting for acknowledge from motors", tsw::utilities::Acknowledge);
    _commandPort->ReadFromDevice(Motors, 0x8f);
    Log("Acknowledge from motors received", tsw::utilities::Acknowledge);
}

void MotorController::ReadSuccess()
{
    Log("Waiting for success response from motors", tsw::io::Acknowledge);
    _commandPort->ReadFromDevice(Motors, 0x81);
    Log("Success response from motors read", tsw::utilities::Acknowledge);
}

int MotorController::AngleToMotorValue(double angle, MotorConfig config)
{
    // Get a value between 0 and 1 for how close to the max angle our given angle is.
    // 1 represents the max angle and 0 represents the min angle.
    // Technically if our value is outside the bounds, this number will be outside [0, 1]
    double angleProp = (angle - config.angleBounds.min) / (config.angleBounds.max - config.angleBounds.min);
    return (int)(config.stepBounds.min + angleProp * (config.stepBounds.max - config.stepBounds.min));
}
