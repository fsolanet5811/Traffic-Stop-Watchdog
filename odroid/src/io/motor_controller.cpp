#include "io.hpp"

using namespace tsw::io;

MotorController::MotorController(DeviceSerialPort& commandPort, MotorConfig panConfig, MotorConfig tiltConfig)
{
    _commandPort = &commandPort;
    PanConfig = panConfig;
    TiltConfig = tiltConfig;
}

void MotorController::SendAsyncRelativeMoveCommand(double horizontal, double vertical)
{
    SendMoveCommand(0xe6, horizontal, vertical, "Async Rel");
}

void MotorController::SendSyncRelativeMoveCommand(double horizontal, double vertical)
{
    SendMoveCommand(0xe5, horizontal, vertical, "Sync Rel");
}

void MotorController::SendAsyncAbsoluteMoveCommand(double horizontal, double vertical)
{
    SendMoveCommand(0xe8, horizontal, vertical, "Async Abs");
}

void MotorController::SendSyncAbsoluteMoveCommand(double horizontal, double vertical)
{
    SendMoveCommand(0xe7, horizontal, vertical, "Sync Abs");
}

void MotorController::SendMoveCommand(uchar specifierByte, double horizontal, double vertical, string moveName)
{
    // Calculate the motor values for each of these.
    int horizontalMotor = AngleToMotorValue(horizontal, PanConfig);
    int verticalMotor = AngleToMotorValue(vertical, TiltConfig);
    
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
    _commandPort->WriteToDevice(0x89);
    Log("Motors Activated", Motors);
}

void MotorController::Deactivate()
{
    Log("Deactivating motors", Motors);
    _commandPort->WriteToDevice(0x8a);
    Log("Motors Deactivated", Motors);
}

bool MotorController::TryReadMessage(DeviceMessage* message)
{
    return _commandPort->TryReadFromDevice(Motors, message);
}

int MotorController::AngleToMotorValue(double angle, MotorConfig config)
{
    // Get a value between 0 and 1 for how close to the max angle our given angle is.
    // 1 represents the max angle and 0 represents the min angle.
    // Technically if our value is outside the bounds, this number will be outside [0, 1]
    double angleProp = (angle - config.angleBounds.min) / (config.angleBounds.max - config.angleBounds.min);
    return (int)(config.stepBounds.min + angleProp * (config.stepBounds.max - config.stepBounds.min));
}