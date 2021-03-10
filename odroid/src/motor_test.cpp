#include "io.hpp"
#include "settings.hpp"

using namespace tsw::io;
using namespace tsw::io::settings;


int main(int argc, char* argv[])
{
    if(argc != 4)
    {
        cout << "Usage: motor_test <motion_type> <horizontal> <vertical>" << endl;
        return 1;
    }
    
    int motionType = atoi(argv[1]);
    double horizontal = atof(argv[2]);
    double vertical = atof(argv[3]);

    string thisFile(argv[0]);
    string startingDir = thisFile.substr(0, thisFile.find_last_of('/'));
    string settingsFile = startingDir + "/motor_test.json";
    MotorSettings settings(settingsFile);
    ConfigureLog(settings.LogFlags);
    SerialPort port;

    port.Open(settings.MotorsSerialPath);

    DeviceSerialPort devicePort(port);
    devicePort.StartGathering();
    MotorController motorController(devicePort, settings.PanConfig, settings.TiltConfig);

    motorController.Activate();

    cout << "Sending move command Type = " << motionType << " H: " << horizontal << " V: " << vertical << endl;
    switch(motionType)
    {
        case RelativeMoveAsynchronous:
            motorController.SendAsyncRelativeMoveCommand(horizontal, vertical);
            break;

        case RelativeMoveSynchronous:
            motorController.SendSyncRelativeMoveCommand(horizontal, vertical);
            break;

        case AbsoluteMoveAsynchronous:
            motorController.SendSyncAbsoluteMoveCommand(horizontal, vertical);
            break;

        case AbsoluteMoveSynchronous:
            motorController.SendSyncAbsoluteMoveCommand(horizontal, vertical);
            break;

        default:
            cout << "Unimplemented motion type " << motionType << endl;
    }

    cout << "Move command sent and finished" << endl;
    return 0;
}