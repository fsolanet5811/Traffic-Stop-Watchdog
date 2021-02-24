#include "imaging.hpp"
#include "io.hpp"
#include "Spinnaker.h"
#include <fstream>
#include "settings.hpp"

using namespace tsw::imaging;
using namespace tsw::io;
using namespace tsw::io::settings;

DeviceSerialPort* ConnectToSerialPort(string serialPath)
{
    SerialPort* rawCommandPort = new SerialPort();
    while(true)
    {
        try
        {
            Log("Opening device serial port on path " + serialPath, Debug | DeviceSerial);
            rawCommandPort->Open(serialPath);
            Log("Device serial port opened", Information | DeviceSerial);
            return new DeviceSerialPort(*rawCommandPort);
        }
        catch(exception e)
        {
            Log("Could not open serial port", tsw::io::Error); 
        }

        // Wait a bit before connecting again.
        sleep(5);
    }
}

CommandAgent* ConnectToDeviceAdapter(string deviceSerialPath)
{
    DeviceSerialPort* commandPort = ConnectToSerialPort(deviceSerialPath);
    return new CommandAgent(*commandPort);
}

MultiPortCommandAgent* ConnectToMultiDevice(string handheldSerialPath, string motorsSerialPath)
{
    DeviceSerialPort* handheldCommandPort = ConnectToSerialPort(handheldSerialPath);
    DeviceSerialPort* motorsCommandPort = ConnectToSerialPort(motorsSerialPath);
    return new MultiPortCommandAgent(*handheldCommandPort, *motorsCommandPort);
}

FlirCamera* ConnectToCamera(string cameraSerialNumber)
{
    FlirCamera* camera = new FlirCamera();;
    while(true)
    {
        try
        {
            Log("Connecting to camera " + cameraSerialNumber, Debug);
            camera->Connect(cameraSerialNumber);
            Log("Camera connected", Information);
            return camera;
        }
        catch(exception e)
        {
            Log("Could not connect to camera. " + string(e.what()), tsw::io::Error);
        }

        // Wait a bit before connecting again.
        sleep(5);
    }
}

void PrintFile(string fileName)
{
    ifstream fs;
    fs.open(fileName);
    string fileLine;
    while(getline(fs, fileLine))
    {
        cout << fileLine <<endl;
    }

    fs.close();
}

int main(int argc, char* argv[])
{
    // Initialize the settings.
    string thisFile(argv[0]);
    string startingDir = thisFile.substr(0, thisFile.find_last_of('/'));
    string settingsFile = startingDir + "/tsw.json";
    Log("Loading settings from " + settingsFile, Information);
    TswSettings settings(settingsFile);
    Log("Settings loaded", Information);
    PrintFile(settingsFile);
    ConfigureLog(settings.LogFlags);

    // Connect to the device port.
    DeviceSerialPort* portThatCanTalkToMotors;
    CommandAgent* agent;
    if(settings.UseDeviceAdapter)
    {
        portThatCanTalkToMotors = ConnectToSerialPort(settings.DeviceSerialPath);
        agent = new CommandAgent(*portThatCanTalkToMotors);
    }
    else
    {
        portThatCanTalkToMotors = ConnectToSerialPort(settings.MotorsSerialPath);
        DeviceSerialPort* handheldPort = ConnectToSerialPort(settings.HandheldSerialPath);
        handheldPort->StartGathering();
        agent = new MultiPortCommandAgent(*handheldPort, *portThatCanTalkToMotors);
    }

    portThatCanTalkToMotors->StartGathering();

    // Connect to the camera and attach the recorder.
    FlirCamera* camera = ConnectToCamera(settings.CameraSerialNumber);
    camera->SetFrameHeight(settings.CameraFrameHeight);
    camera->SetFrameWidth(settings.CameraFrameWidth);
    camera->SetFrameRate(settings.CameraFrameRate);
    Recorder recorder(*camera);

    // Setup the motion control.
    ConfidenceOfficerLocator officerLocator(settings.OfficerClassId);
    officerLocator.TargetRegionProportion = settings.TargetRegionProportion;
    officerLocator.SafeRegionProportion = settings.SafeRegionProportion;
    CameraMotionController motionController(*camera, officerLocator, *portThatCanTalkToMotors);
    motionController.CameraFramesToSkip = settings.CameraFramesToSkipMoving;
    motionController.PanConfig = settings.PanConfig;
    motionController.TiltConfig = settings.TiltConfig;

    // Now here comes the actual processing.
    // For now, if it messes up, we will just display an error and 
    try
    {
        while(true)
        {
            // Read a command from the handheld device and acknowledge it.
            Log("Waiting for command", Information | DeviceSerial);
            Command* command = agent->ReadCommand(Handheld);
            agent->AcknowledgeReceived(Handheld);

            // See what the command wants us to do.
            switch(command->action)
            {
                case StartOfficerTracking:
                    Log("Starting officer tracking", Information | DeviceSerial | Recording | Officers);
                    motionController.StartCameraMotionGuidance();
                    camera->StartLiveFeed();
                    recorder.StartRecording("1footage.avi");                    
                    break;

                case StopOfficerTracking:
                    Log("Stopping officer tracking", Information | DeviceSerial | Recording | Officers);
                    camera->StopLiveFeed();
                    motionController.StopCameraMotionGuidance();
                    recorder.StopRecording();
                    break;

                default:
                    Log("Unimplemented command " + to_string(command->action), tsw::io::Error | DeviceSerial);
            }

            // Gotta dealocate!
            delete command;
        }
    }
    catch(exception ex)
    {
        Log("An error occured:\n" + string(ex.what()), tsw::io::Error);
    }
    
    if(camera->IsLiveFeedOn())
    {
        camera->StopLiveFeed();
    }
    
    if(recorder.IsRecording())
    {
        recorder.StopRecording();
    }
      
    if(motionController.IsGuidingCameraMotion())
    {
        motionController.StopCameraMotionGuidance();
    }

    delete agent;

    return 0;  
}
