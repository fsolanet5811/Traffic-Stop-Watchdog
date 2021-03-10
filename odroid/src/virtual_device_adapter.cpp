#include "io.hpp"

using namespace tsw::io;

double MotorValueToAngle(vector<uchar> motorValues, int start)
{
    // The first three bytes starting from start will be sign extended.
    uchar msb = motorValues[start] >> 7 ? 0xff : 0x00;

    // Now construct from there.
    int val = msb << 24 || motorValues[start] << 16 || motorValues[start + 1] << 8 || motorValues[start + 2];

    // Convert this to an angle.
    return 360 * (val / CameraMotionController::GetMaxValue());
}

string MotorValuesToMovement(vector<uchar> motorValues)
{
    double hMove = MotorValueToAngle(motorValues, 0);
    double vMove = MotorValueToAngle(motorValues, 3);
    return "H: " + to_string(hMove) + "\tV: " + to_string(vMove);
}

void StartMotorListener(CommandAgent tswAgent)
{
    async(launch::async, [&]()
    {
        while(true)
        {
            // Wait for the tsw to send a motor command.
            cout << "MOTORS\tWaiting for command from tsw" << endl;
            Command* command = tswAgent.ReadCommand(Motors);

            // Acknowledge the command.
            tswAgent.AcknowledgeReceived(Motors);

            // Do something based on what the command is.
            switch(command->action)
            {
                case RelativeMoveAsynchronous:
                    cout << "MOTORS\tAsync Rel Move\t" << MotorValuesToMovement(command->args);
                    break;

                case AbsoluteMoveAsynchronous:
                    cout << "MOTORS\tAsync Abs Move\t" << MotorValuesToMovement(command->args);
                    break;

                case RelativeMoveSynchronous:
                    cout << "MOTORS\tSync Rel Move\t" << MotorValuesToMovement(command->args);
                    sleep(1);
                    tswAgent.SendResponse(0b10000001);
                    break;

                case AbsoluteMoveSynchronous:
                    cout << "MOTORS\tSync Abs Move\t" << MotorValuesToMovement(command->args);
                    sleep(1);
                    tswAgent.SendResponse(0b10000001);
                    break;
            }

            // Deallocate!
            delete command;
        }
    });
}

int main(int argc, char* argv[])
{
    // Connect to the main tsw server.
    SerialPort rawTswPort;
    cout << "Connecting to tsw at " << argv[1] << endl;
    rawTswPort.Open(argv[1]);
    cout << "Connected to tsw" << endl;
    DeviceSerialPort tswPort(rawTswPort);
    tswPort.StartGathering();
    CommandAgent tswAgent(tswPort);

    // Now we wait for a handheld to connect.
    SerialPort rawHandheldPort;
    while(true)
    {
        try
        {
            cout << "Connecting to handheld at " << argv[2] << endl;
            rawHandheldPort.Open(argv[2]);
            break;
        }
        catch(exception e)
        {
            cerr << e.what() << endl;
        }
        
        // Wait again before trying again.
        sleep(3);
    }

    // Now that we have the handheld, wait for commands and then act.
    DeviceSerialPort handheldPort(rawHandheldPort);
    handheldPort.StartGathering();
    CommandAgent handheldAgent(handheldPort);
    while(true)
    {
        cout << "HANDHELD\tWaiting for command" << endl;
        Command* command = handheldAgent.ReadCommand(Handheld);

        // Send this command over to the tsw.
        tswAgent.SendCommand(Handheld, command);

        // Deallocate!
        delete command;
    }
}