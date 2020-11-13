#include "imaging.hpp"
#include "io.hpp"
#include "Spinnaker.h"

using namespace tsw::imaging;
using namespace tsw::io;

SerialPort* ConnectToDevice(string deviceSerialPath)
{
    SerialPort* rawCommandPort = new SerialPort();
    while(true)
    {
        try
        {
            Log("Opening device serial port on path " + deviceSerialPath, Debug | DeviceSerial);
            rawCommandPort->Open(deviceSerialPath);
            Log("Device serial port opened", Information | DeviceSerial);
            break;
        }
        catch(exception e)
        {
            Log("Could not open serial port", tsw::io::Error); 
        }

        // Wait a bit before connecting again.
        sleep(5);
    }
    
    return rawCommandPort;
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

int main(int argc, char* argv[])
{
    // Initialize the settings.
    string thisFile(argv[0]);
    string startingDir = thisFile.substr(0, thisFile.find_last_of('/'));
    string settingsFile = startingDir + "/tsw.json";
    Log("Loading settings from " + settingsFile, Information);
    Settings settings(settingsFile);
    Log("Settings loaded", Information);
    ConfigureLog(settings.LogFlags);

    // Connect to the device port.
    SerialPort* rawCommandPort = ConnectToDevice(settings.DeviceSerialPath);
    DeviceSerialPort commandPort(*rawCommandPort);
    commandPort.StartGathering();
    CommandAgent agent(commandPort);

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
    CameraMotionController motionController(*camera, officerLocator, commandPort);
    motionController.CameraFramesToSkip = settings.CameraFramesToSkipMoving;

    // Now here comes the actual processing.
    // For now, if it messes up, we will just display an error and 
    try
    {
        while(true)
        {
            // Read a command from the handheld device and acknowledge it.
            Log("Waiting for command", Information | DeviceSerial);
            Command* command = agent.ReadCommand(Handheld);
            agent.AcknowledgeReceived(Handheld);

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
                    motionController.StopCameraMotionGuidance();
                    recorder.StopRecording();
                    camera->StopLiveFeed();
                    break;

                default:
                    Log("Unimplemented command " + command->action, tsw::io::Error | DeviceSerial);
            }

            // Gotta dealocate!
            delete command;
        }
    }
    catch(exception ex)
    {
        Log("An error occured:\n" + string(ex.what()), tsw::io::Error);
    }
    
    // Stop all of the devices.
    commandPort.StopGathering();
    rawCommandPort->Close();

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
