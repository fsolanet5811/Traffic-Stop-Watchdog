#include "imaging.hpp"
#include "io.hpp"
#include "Spinnaker.h"

using namespace tsw::imaging;
using namespace tsw::io;

int main()
{
    // Initialize the settings.
    string settingsFile = "tsw.config";
    cout << "Loading settings from " << settingsFile << endl;
    Settings settings("tsw.config");
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
    cout << "Connecting to camera " << settings.CameraSerialNumber << endl;
    FlirCamera camera;    
    camera.Connect(settings.CameraSerialNumber);
    cout << "Camera connected" << endl;
    Recorder recorder(camera);

    // Setup the motion control.
    cout << "Using officer class id " << settings.OfficerClassId << endl;
    ConfidenceOfficerLocator officerLocator(settings.OfficerClassId);
    CameraMotionController motionController(camera, officerLocator, commandPort);

    // Now here comes the actual processing.
    // For now, if it messes up, we will just display an error and 
    try
    {
        while(true)
        {
            // Read a command from the handheld device and acknowledge it.
            Command* command = agent.ReadCommand(Handheld);
            agent.AcknowledgeReceived(Handheld);

            // See what the command wants us to do.
            switch(command->action)
            {
                case StartOfficerTracking:
                    cout << "Starting officer tracking" << endl;
                    camera.StartLiveFeed();
                    recorder.StartRecording("footage.avi");
                    motionController.StartCameraMotionGuidance();
                    break;

                case StopOfficerTracking:
                    cout << "Stopping officer tracking" << endl;
                    motionController.StopCameraMotionGuidance();
                    recorder.StopRecording();
                    camera.StopLiveFeed();
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

    if(camera.IsLiveFeedOn())
    {
        camera.StopLiveFeed();
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