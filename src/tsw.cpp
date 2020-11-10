#include "imaging.hpp"
#include "io.hpp"
#include "Spinnaker.h"

using namespace tsw::imaging;
using namespace tsw::io;

DeviceSerialPort* ConnectToDevice(string deviceSerialPath)
{
    SerialPort rawCommandPort;
    while(true)
    {
        try
        {
            cout << "Opening device serial port on path " << deviceSerialPath << endl;
            rawCommandPort.Open(deviceSerialPath);
            cout << "Device serial port opened" << endl;
            break;
        }
        catch(exception e)
        {
            cout << "Could not open serial port" << endl; 
        }

        // Wait a bit before connecting again.
        sleep(5);
    }
    
    // Now we can hook up the device wrapper and start reading from the port.
    DeviceSerialPort* commandPort = new DeviceSerialPort(rawCommandPort);
    commandPort->StartGathering();
    return commandPort;
}

FlirCamera* ConnectToCamera(string cameraSerialNumber)
{
    FlirCamera* camera = new FlirCamera();;
    while(true)
    {
        try
        {
            cout << "Connecting to camera " << cameraSerialNumber << endl;
            camera->Connect(cameraSerialNumber);
            cout << "Camera connected" << endl;
            camera->SetFrameHeight(480);
            camera->SetFrameWidth(720);
            camera->SetFrameRate(25);
            return camera;
        }
        catch(exception e)
        {
            cout << "Could not connect to camera. " << e.what() << endl;
        }

        // Wait a bit before connecting again.
        sleep(5);
    }
}

int main(int argc, char* argv[])
{
    // Initialize the settings.
    string thisFile(argv[0]);
    string startingDir = thisFile.substr(0, thisFile.find_last_of('/'));
    string settingsFile = startingDir + "/tsw.json";
    cout << "Loading settings from " << settingsFile << endl;
    Settings settings(settingsFile);
    cout << "Settings loaded" << endl;

    // Connect to the device port.
    SerialPort rawCommandPort;
    cout << "Opening device serial port on path " << settings.DeviceSerialPath << endl;
    rawCommandPort.Open(settings.DeviceSerialPath);
    cout << "Device serial port opened" << endl;
    DeviceSerialPort commandPort(rawCommandPort);
    commandPort.StartGathering();
    CommandAgent agent(commandPort);

    // Connect to the camera and attach the recorder.
    FlirCamera* camera = ConnectToCamera(settings.CameraSerialNumber);
    Recorder recorder(*camera);

    // Setup the motion control.
    ConfidenceOfficerLocator officerLocator(settings.OfficerClassId);
    officerLocator.TargetRegionProportion = settings.TargetRegionProportion;
    officerLocator.SafeRegionProportion = settings.SafeRegionProportion;
    CameraMotionController motionController(*camera, officerLocator, commandPort);
    motionController.CameraFramesToSkip = settings.CameraFramesToSkipMoving;

    // Now here comes the actual processing.
    // For now, if it messes up, we will just display an error and 
    try
    {
        while(true)
        {
            // Read a command from the handheld device and acknowledge it.
            cout << "Waiting for command" << endl;
            Command* command = agent.ReadCommand(Handheld);
            agent.AcknowledgeReceived(Handheld);

            // See what the command wants us to do.
            switch(command->action)
            {
                case StartOfficerTracking:
                    cout << "Starting officer tracking" << endl;
                    camera->StartLiveFeed();
                    recorder.StartRecording("1footage.avi");
                    motionController.StartCameraMotionGuidance();
                    break;

                case StopOfficerTracking:
                    cout << "Stopping officer tracking" << endl;
                    motionController.StopCameraMotionGuidance();
                    recorder.StopRecording();
                    camera->StopLiveFeed();
                    break;

                default:
                    cout << "Unimplemented command " << command->action << endl;
            }

            // Gotta dealocate!
            delete command;
        }
    }
    catch(exception ex)
    {
        cout << endl << "An error occured:" << endl << ex.what() << endl;
    }
    
    // Stop all of the devices.
    commandPort.StopGathering();
    rawCommandPort.Close();

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

    return 0;  
}