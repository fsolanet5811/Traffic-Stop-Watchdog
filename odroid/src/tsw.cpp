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

FlirCamera* ConnectToCamera(TswSettings& settings)
{
    FlirCamera* camera = new FlirCamera(settings.CameraBufferCount);
    while(true)
    {
        try
        {
            Log("Connecting to camera " + settings.CameraSerialNumber, Debug);
            camera->Connect(settings.CameraSerialNumber);

            // Configuring the camera is now part of the setup.
            Log("Setting camera parameters", Debug);
            camera->SetFrameHeight(settings.CameraFrameHeight);
            camera->SetFrameWidth(settings.CameraFrameWidth);
            camera->SetFrameRate(settings.CameraFrameRate);
            Log("Camera parameters set", Debug);

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

    // We only need the live feed if we actually are going to move and/or record.
    if(settings.ImagingConfig.moveCamera || settings.ImagingConfig.recordFrames || settings.ImagingConfig.displayFrames)
    {
        // Start the processing first so that everything is setup for when we get the first frame.
        imageProcessor.StartProcessing();
        camera->StartLiveFeed();
    }
    
    Log("Officer tracking started", Information | DeviceSerial | Recording | Officers);
}

void FinishOfficerTracking(CameraMotionController& motionController, FlirCamera* camera, ImageProcessor& imageProcessor, TswSettings& settings, StatusLED& led)
{
    Log("Stopping officer tracking", Information | DeviceSerial | Recording | Officers);

    if(settings.ImagingConfig.moveCamera || settings.ImagingConfig.recordFrames || settings.ImagingConfig.displayFrames)
    {
        // Start the processing first so that everything is setup for when we get the first frame.
        led.FlashesPerPause = 4;
        imageProcessor.StopProcessing();
        led.FlashesPerPause = 5;
        camera->StopLiveFeed();
    }

    Log("Officer tracking stopped", Information | DeviceSerial | Recording | Officers);
}

int main(int argc, char* argv[])
{
    // Initialize the settings.
    // I wish we could do this after setting up the led, but we need the settings to set it up lol.
    string thisFile(argv[0]);
    string startingDir = thisFile.substr(0, thisFile.find_last_of('/'));
    string settingsFile = startingDir + "/tsw.json";
    Log("Loading settings from " + settingsFile, Information);
    TswSettings settings(settingsFile);
    Log("Settings loaded", Information);    

    // Setup the led as early as possible.
    StatusLED led(settings.StatusLEDFile);
    led.SetEnabled(settings.UseStatusLED);

    // The initial flash setup is just one flash.
    // I want to do this now in case we have a problem writing to the output.
    // Since the app is going to be ran headless, we will try to use this to get a sense of what is going on in the program.
    led.StartFlashing(1);

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

    // Serial port setup is done.
    led.FlashesPerPause = 2;

    // Connect to the camera and attach the recorder and display window.
    FlirCamera* camera = ConnectToCamera(settings);    

    Size frameSize;
    frameSize.height = camera->GetFrameHeight();
    frameSize.width = camera->GetFrameWidth();
    Recorder recorder(frameSize, camera->GetFrameRate());
    DisplayWindow window("Officer Footage", settings.FrameDisplayRefreshRate);

    // Setup the motion control.
    SmartOfficerLocator officerLocator(settings.OfficerClassId);
    officerLocator.TargetRegionProportion = settings.TargetRegionProportion;
    officerLocator.SafeRegionProportion = settings.SafeRegionProportion;
    officerLocator.ConfidenceThreshold = settings.OfficerConfidenceThreshold;
    officerLocator.MaxHSV = settings.MaxOfficerHSV;
    officerLocator.MinHSV = settings.MinOfficerHSV;
    officerLocator.OfficerThreshold = settings.OfficerThreshold;

    // These two guys will handle moving the system.
    MotorController motorController(*portThatCanTalkToMotors, settings.PanConfig, settings.TiltConfig);
    CameraMotionController motionController(motorController);
    motionController.HomeAngles = settings.HomeAngles;
    motionController.AngleXBounds = settings.AngleXBounds;
    motionController.MotorSpeeds = settings.MotorSpeeds;

    // The FOV changes based on the resolution.
    motionController.CalibrateFOV(settings.CameraFrameWidth, settings.CameraFrameHeight);

    // This will handle displaying and recording when we get images.
    ImageProcessor imageProcessor(window, *camera, officerLocator, motionController, settings.ImagingConfig);
    imageProcessor.CameraFramesToSkip = settings.CameraFramesToSkipMoving;

    // This will mark that we are just chilling.
    led.FlashesPerPause = 3;

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
                    // A slower pause will have us writing less to the disk to max processing on the images.
                    led.FlashesPerPause = 1;
                    led.PauseTime = 2000000;
                    RunOfficerTracking(motionController, camera, imageProcessor, settings);                    
                    break;

                case StopOfficerTracking:
                    // Decreae the super long pause.
                    led.PauseTime = 750000;
                    FinishOfficerTracking(motionController, camera, imageProcessor, settings, led);
                    led.FlashesPerPause = 3;
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
    
    if(imageProcessor.IsProcessing())
    {
        imageProcessor.StopProcessing();
    }
    
    delete agent;
    delete camera;

    return 0;  
}
