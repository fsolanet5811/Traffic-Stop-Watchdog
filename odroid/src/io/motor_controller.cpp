#include "io.hpp"
#include "utilities.hpp"

using namespace tsw::io;
using namespace tsw::utilities;

MotorController::MotorController(DeviceSerialPort& commandPort, MotorConfig panConfig, MotorConfig tiltConfig)
{
    _commandPort = &commandPort;
    PanConfig = panConfig;
    TiltConfig = tiltConfig;
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
    vector<uchar> bytes(6);
    for(int i = 0; i < 3; i++)
    {
        bytes[2 - i] = (horizontalMotor >> (i * 8)) & 0xff;
        bytes[5 - i] = (verticalMotor >> (i * 8)) & 0xff;
    }

    // Now we can send the command to the motor.
    // This is an asynchronous move since we don't really need to know when the motor has moved.
    Log("MOVE " + moveName + "\tH:  " + to_string(horizontal) + "\tV:  " + to_string(vertical), Movements);
    _commandPort->WriteToDevice(Motors, moveType, bytes);

    // Wait for the acknowledge (not the same as a synch response).
    // It is possible that the read response is not an ack but a success/failure from a previous move.
    while(true)
    {
        Log("Reading move command acknowledge", tsw::utilities::Acknowledge);
        DeviceMessage message = _commandPort->ReadFromDevice(Motors);

        // Acknowledgements will be four 1's in the lsbs.
        uchar lsbs = message.bytes[0] & 0x0f;
        if(lsbs == 0x0f)
        {
            // This is the acknowledgement.
            Log("Move command acknowledge received", tsw::utilities::Acknowledge);
            return;
        }

        if(lsbs == 0x02)
        {
            // There was a fault in the motors' last movement.
            Log("Motors returned FAULT!", tsw::utilities::Error);
        }
        else
        {
            // The byte was hopefully a success byte, just read another one.
            Log("Move command success response read", Acknowledge);
        }
    }
    
}

void MotorController::Activate()
{
    Log("Activating motors", Motors);
    _commandPort->WriteToDevice(Motors, tsw::io::Activate);
	
	// Read the acknowledgement.
    Log("Waiting for acknowledge from motors", Motors | Acknowledge);
    _commandPort->ReadFromDevice(Motors);
    Log("Acknowledge from motors received", Motors | Acknowledge);

	// The motors will send us a byte when it is calibrated.
	// We gotta wait for this before starting anything else.
    Log("Waiting for activation finish response", Motors);
	_commandPort->ReadFromDevice(Motors);
    Log("Motors Activated", Motors);
}

void MotorController::Deactivate()
{
    Log("Deactivating motors", Movements);
    _commandPort->WriteToDevice(Motors, tsw::io::Deactivate);
    
    // Read the acknowledgement.
    Log("Waiting for acknowledge from motors", Movements | tsw::utilities::Acknowledge);
    _commandPort->ReadFromDevice(Motors);
    Log("Acknowledge from motors received", Movements | tsw::utilities::Acknowledge);

    Log("Motors Deactivated", Motors);
}

bool MotorController::TryReadMessage(DeviceMessage* message)
{
    return _commandPort->TryReadFromDevice(Motors, message);
}

void MotorController::SetSpeeds(ByteVector2 speeds)
{
    // To keep it consistent, we will send the horizontal speed first.
    vector<uchar> data;
    data.push_back(speeds.x);
    data.push_back(speeds.y);

    // Now send the command.
    Log("Settings motor speeds to (" + to_string((int)speeds.x) + ", " + to_string((int)speeds.y) + ")", Movements);
    _commandPort->WriteToDevice(Motors, tsw::io::SetSpeeds, data);

    // They will send an ack when they get it.
    Log("Waiting for set speed acknowledge from motors", tsw::utilities::Acknowledge);
    _commandPort->ReadFromDevice(Motors);
    Log("Set speed acknowledge received", tsw::utilities::Acknowledge);

    Log("Motor speeds set", Movements);
}

int MotorController::AngleToMotorValue(double angle, MotorConfig config)
{
    // Get a value between 0 and 1 for how close to the max angle our given angle is.
    // 1 represents the max angle and 0 represents the min angle.
    // Technically if our value is outside the bounds, this number will be outside [0, 1]
    double angleProp = (angle - config.angleBounds.min) / (config.angleBounds.max - config.angleBounds.min);
    return (int)(config.stepBounds.min + angleProp * (config.stepBounds.max - config.stepBounds.min));
}
