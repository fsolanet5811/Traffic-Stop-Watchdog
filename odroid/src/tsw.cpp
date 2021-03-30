#include "imaging.hpp"
#include "io.hpp"
#include "Spinnaker.h"
#include "utilities.hpp"
#include <fstream>
#include "settings.hpp"
    
using namespace tsw::imaging;
using namespace tsw::io;
using namespace tsw::io::settings;
using namespace tsw::utilities;

DeviceSerialPort* ConnectToSerialPort(string serialPath, speed_t baudRate)
{
    SerialPort* rawCommandPort = new SerialPort(baudRate);
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
            Log("Could not open serial port", tsw::utilities::Error); 
        }

        // Wait a bit before connecting again.
        sleep(5);
    }
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
            Log("Could not connect to camera. " + string(e.what()), tsw::utilities::Error);
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

void RunOfficerTracking(CameraMotionController& motionController, FlirCamera* camera, ImageProcessor& imageProcessor, TswSettings& settings)
{
    Log("Starting officer tracking", Information | DeviceSerial | Recording | Officers);
    
    if(settings.MoveCamera)
    {
        motionController.StartCameraMotionGuidance();
    }
    
    // We only need the live feed if we actually are going to move and/or record.
    if(settings.MoveCamera || settings.ImagingConfig.recordFrames || settings.ImagingConfig.displayFrames)
    {
        // Start the processing first so that everything is setup for when we get the first frame.
        imageProcessor.StartProcessing();
        camera->StartLiveFeed();
    }
    
    Log("Officer tracking started", Information | DeviceSerial | Recording | Officers);
}

void FinishOfficerTracking(CameraMotionController& motionController, FlirCamera* camera, ImageProcessor& imageProcessor, TswSettings& settings)
{
    Log("Stopping officer tracking", Information | DeviceSerial | Recording | Officers);
           
    if(settings.MoveCamera)
    {
        motionController.StopCameraMotionGuidance();
    }
    
    if(settings.MoveCamera || settings.ImagingConfig.recordFrames || settings.ImagingConfig.displayFrames)
    {
        // Start the processing first so that everything is setup for when we get the first frame.
        imageProcessor.StopProcessing();
        camera->StopLiveFeed();
    }

    Log("Officer tracking stopped", Information | DeviceSerial | Recording | Officers);
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
        portThatCanTalkToMotors = ConnectToSerialPort(settings.DeviceSerialConfig.path, settings.DeviceSerialConfig.baudRate);
        agent = new CommandAgent(*portThatCanTalkToMotors);
    }
    else
    {
        portThatCanTalkToMotors = ConnectToSerialPort(settings.MotorsSerialConfig.path, settings.MotorsSerialConfig.baudRate);
        DeviceSerialPort* handheldPort = ConnectToSerialPort(settings.HandheldSerialConfig.path, settings.HandheldSerialConfig.baudRate);
        handheldPort->StartGathering();
        agent = new CommandAgent(*handheldPort);
    }

    portThatCanTalkToMotors->StartGathering();

    // Connect to the camera and attach the recorder and display window.
    FlirCamera* camera = ConnectToCamera(settings.CameraSerialNumber);
    camera->SetFrameHeight(settings.CameraFrameHeight);
    camera->SetFrameWidth(settings.CameraFrameWidth);
    camera->SetFrameRate(settings.CameraFrameRate);

    Size frameSize;
    frameSize.height = camera->GetFrameHeight();
    frameSize.width = camera->GetFrameWidth();
    Recorder recorder(frameSize, camera->GetFrameRate());
    DisplayWindow window("Officer Footage", settings.FrameDisplayRefreshRate);
    ImageProcessor imageProcessor(recorder, window, *camera, settings.ImagingConfig);

    // Setup the motion control.
    ConfidenceOfficerLocator officerLocator(settings.OfficerClassId);
    officerLocator.TargetRegionProportion = settings.TargetRegionProportion;
    officerLocator.SafeRegionProportion = settings.SafeRegionProportion;

    MotorController motorController(*portThatCanTalkToMotors, settings.PanConfig, settings.TiltConfig);

    CameraMotionController motionController(*camera, officerLocator, motorController);
    motionController.CameraFramesToSkip = settings.CameraFramesToSkipMoving;
    motionController.HomeAngles = settings.HomeAngles;

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
                    RunOfficerTracking(motionController, camera, imageProcessor, settings);                    
                    break;

                case StopOfficerTracking:
                    FinishOfficerTracking(motionController, camera, imageProcessor, settings);
                    break;

                case SendKeyword:
                    Log("Received Keyword: " + string(command->args.begin(), command->args.end()), Information);
                    break;

                default:
                    Log("Unimplemented command " + to_string(command->action), tsw::utilities::Error | DeviceSerial);
            }

            // Gotta dealocate!
            delete command;
        }
    }
    catch(exception ex)
    {
        Log("An error occured:\n" + string(ex.what()), tsw::utilities::Error);
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
