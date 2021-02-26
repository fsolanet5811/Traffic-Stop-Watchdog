#include "io.hpp"
#include "settings.hpp"

using namespace tsw::io;
using namespace tsw::io::settings;


int main(int argc, char* argv[])
{    
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

    motorController.Deactivate();
    return 0;
}