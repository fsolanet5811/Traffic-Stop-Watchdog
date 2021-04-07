#include "SpinGenApi/SpinnakerGenApi.h"
#include "Spinnaker.h"
#include "imaging.hpp"
#include "io.hpp"
#include <iostream>
#include <sstream>
#include <future>
#include <thread>
#include <chrono>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace tsw::imaging;
using namespace tsw::io;
using namespace std;

class LoggingCallback : Spinnaker::LoggingEventHandler
{
    void OnLogEvent(LoggingEventDataPtr dataPtr)
    {
        stringstream ss;
        ss << dataPtr->GetPriorityName() << " | " << dataPtr->GetLogMessage();
        Log(ss.str(), Flir);
    }
};

FlirCamera::FlirCamera(int bufferCount)
{
    LoggingCallback* logCallback = new LoggingCallback();
    _system = System::GetInstance();
    _system->RegisterLoggingEventHandler((LoggingEventHandler&)*logCallback);  
    _system->SetLoggingEventPriorityLevel(Spinnaker::LOG_LEVEL_DEBUG);
    _nextLiveFeedKey = 1;
    _isLiveFeedOn = false;
    _liveFeedLock.Name = "LFD";
    _bufferCount = bufferCount;

    // These are the settings that the caller desires. Null values indicate that they have not been set yet.
    _userFrameRate = nullptr;
    _userFrameWidth = nullptr;
    _userFrameHeight = nullptr;
    _userFilter = nullptr;
}

FlirCamera::~FlirCamera()
{
    // Make sure the live feed is not going.
    StopLiveFeed();

    _system->ReleaseInstance();
    delete _userFilter;
    delete _userFrameHeight;
    delete _userFrameRate;
    delete _userFrameWidth;
}

vector<string> FlirCamera::FindDevices()
{
    _system->UpdateCameras();
    CameraList cameras = _system->GetCameras();
    vector<string> serials;
    for(int i = 0; i < cameras.GetSize(); i++)
    {
        serials.push_back(cameras.GetByIndex(i)->DeviceID.GetValue().c_str());
    }

    return serials;
}

void FlirCamera::Connect(string serialNumber)
{
    if(!TryConnect(serialNumber, &_camera))
    {
        throw runtime_error("Could not find camera with serial number: " + serialNumber + ".");
    }

    _shouldBeConnected = true;
}

double FlirCamera::GetDeviceTemperature()
{
    EnsureConnectionNotLost();
    return _camera->DeviceTemperature.GetValue();
}

double FlirCamera::GetFrameRate()
{
    EnsureConnectionNotLost();
    return _camera->AcquisitionResultingFrameRate.GetValue();
}

int FlirCamera::GetFrameHeight()
{
    EnsureConnectionNotLost();
    return _camera->Height.GetValue();
}

int FlirCamera::GetFrameWidth()
{
    EnsureConnectionNotLost();
    return _camera->Width.GetValue();
}

void FlirCamera::SetFrameRate(double hertz)
{
    EnsureConnectionNotLost();
    _camera->AcquisitionFrameRateEnable = true;
    _camera->AcquisitionFrameRate = hertz;
    delete _userFrameRate;
    _userFrameRate = new double(hertz);
}

void FlirCamera::SetFilter(RgbTransformLightSourceEnums filter)
{
    EnsureConnectionNotLost();
    _camera->RgbTransformLightSource = filter;
    delete _userFilter;
    _userFilter = new RgbTransformLightSourceEnums(filter);
}

void FlirCamera::StartLiveFeed()
{
    // This ensures that we don't have multiple livefeed instances started.
    if(!IsLiveFeedOn())
    {
        Log("Starting camera live feed", Frames);
        _isLiveFeedOn = true;
        _liveFeedFuture = async(launch::async, [this]()
        {
            RunLiveFeed();
        });
    }
}

void FlirCamera::StopLiveFeed()
{
    if(IsLiveFeedOn())
    {
        Log("Stopping camera live feed", Frames);
        _isLiveFeedOn = false;

        // This will wait for the live feed thread to stop.
        _liveFeedFuture.wait();
    }
}

bool FlirCamera::IsLiveFeedOn()
{
    return _isLiveFeedOn;
}

uint FlirCamera::RegisterLiveFeedCallback(function<void(LiveFeedCallbackArgs)> callback)
{
    _liveFeedLock.Lock("Register Callback");

    // This will create a key entry for this callback.
    LiveFeedCallback cb;
    cb.callback = callback;
    cb.callbackKey = _nextLiveFeedKey++;
    _liveFeedCallbacks.push_back(cb);
    _liveFeedLock.Unlock("Register Callback");
    
    // Give them back the key so they can remove the callback later.
    return cb.callbackKey;
}

void FlirCamera::UnregisterLiveFeedCallback(uint callbackKey)
{
    _liveFeedLock.Lock("Unregister Callback");
    _liveFeedCallbacks.remove_if([callbackKey](LiveFeedCallback cb)
    {
        return cb.callbackKey == callbackKey;
    });
    _liveFeedLock.Unlock("Unregister Callback");
}

void FlirCamera::OnLiveFeedImageReceived(ImagePtr image, uint imageIndex)
{
    Log("Frame # " + to_string(imageIndex) + " acquired", Frames);
    LiveFeedCallbackArgs args;
    args.image = image;
    args.imageIndex = imageIndex;

    // We do not want to call any of the callbacks while one of them is being removed/added.
    Log("Starting live feed callbacks", Frames);
    _liveFeedLock.Lock("Run Callback");
    for(LiveFeedCallback cb : _liveFeedCallbacks)
    {
        Log("Calling callback", Frames);
        cb.callback(args);
        Log("Callback finished", Frames);
    }

    // Now we can mess with the callback list.
    _liveFeedLock.Unlock("Run Callback");
    imageIndex++;
    Log("Finished live feed callbacks", Frames);
}

ImagePtr FlirCamera::CaptureImage()
{
    if(IsLiveFeedOn())
    {
        throw runtime_error("Cannot capture single image while live feed is on.");
    }

    EnsureConnectionNotLost();
    _camera->BeginAcquisition();
    ImagePtr image = _camera->GetNextImage();
    ImagePtr convertedImage = image->Convert(PixelFormat_RGB8);
    image->Release();
    _camera->EndAcquisition();
    return convertedImage; 
}

void FlirCamera::RunLiveFeed()
{   
    EnsureConnectionNotLost();

    // Begin acquireung camera frames.
    _camera->BeginAcquisition();

    // This will keep track of which frame we are on.
    uint imageIndex = 0;
    while(IsLiveFeedOn())
    {
        ImagePtr image;
        try
        {
            // Grab an image from the camera
            image = _camera->GetNextImage(1000);
        }
        catch(Spinnaker::Exception e)
        {
            // Inform that we had trouble grabbing an image.
            Log("Error thrown grabbing frame. Live feed will restart. " + string(e.what()), Frames | tsw::utilities::Error);

            // Reset the camera.
            Log("Stopping frame acquisition temporarily", Frames);
            _camera->EndAcquisition();
            PowerCycle();
            
            Log("Frame acquisition resuming", Frames);
            _camera->BeginAcquisition();

            Log("Livefeed restarted successfully.", Frames);
            continue;
        }
        
        // Convert the image so that we can free up the camera buffer for the next frame.
        ImagePtr convertedImage = image->Convert(PixelFormat_RGB8);
        
        // Free up the image from the camera buffer.
        image->Release();

        // Let everybody know that we have received a new image.
        OnLiveFeedImageReceived(convertedImage, imageIndex++);
    }

    // Stop grabbing frames from the camera.
    _camera->EndAcquisition();
}

void FlirCamera::SetFrameHeight(int frameHeight)
{
    Log("Changing camera frame height to " + to_string(frameHeight), Debug | Frames);
    _camera->Height.SetValue(frameHeight);


    // The camera does not auto center the region of interest, so we have to do it manually.
    _camera->OffsetY = (_camera->HeightMax.GetValue() - frameHeight) / 2;

    delete _userFrameHeight;
    _userFrameHeight = new int(frameHeight);
    Log("Camera frame height changed", Information | Frames);
}

void FlirCamera::SetFrameWidth(int frameWidth)
{
    Log("Changing camera frame width to " + to_string(frameWidth), Debug | Frames);
    _camera->Width.SetValue(frameWidth);
    
    // The camera does not auto center the region of interest, so we have to do it manually.
    _camera->OffsetX = (_camera->WidthMax.GetValue() - frameWidth) / 2;

    delete _userFrameWidth;
    _userFrameWidth = new int(frameWidth);
    Log("Camera frame width changed", Information | Frames);
}

bool FlirCamera::TryConnect(string serialNumber, CameraPtr* camera)
{
    Log("Trying to connect to camera " + serialNumber, Frames);

    // Grab hold of the list of cameras from the system.
    _system->UpdateCameras();
    CameraList cameras = _system->GetCameras();
    
    // This will grab the camera that has the given serial number.
    CameraPtr connectedCamera = cameras.GetBySerial(serialNumber);
    if(!connectedCamera)
    {
        Log("Camera not found", Frames);
        return false;
    }
    
    connectedCamera->Init();

    // These are settings that we want on the camera always.
    connectedCamera->ExposureAuto = ExposureAuto_Continuous;
    connectedCamera->GainAuto = GainAuto_Continuous;
    connectedCamera->AcquisitionMode = AcquisitionMode_Continuous;

    // Object detection settings.
    connectedCamera->ChunkEnable = true;
    connectedCamera->ChunkModeActive = true;
    connectedCamera->RgbTransformLightSource = RgbTransformLightSource_General;
    
    // This will make it so that we always grab the newest image.
    INodeMap& nodeMap = connectedCamera->GetTLStreamNodeMap();
    CEnumerationPtr bufferNode = nodeMap.GetNode("StreamBufferHandlingMode");
    bufferNode->SetIntValue(StreamBufferHandlingMode_NewestOnly);

    // For some reason they only give us a few buffers, We want more!
    CEnumerationPtr bufferModeNode = nodeMap.GetNode("StreamBufferCountMode");
    bufferModeNode->SetIntValue(StreamBufferCountMode_Manual);
    CIntegerPtr bufferMaxNode = nodeMap.GetNode("StreamBufferCountMax");
    int maxBuffers = bufferMaxNode->GetValue();
    CIntegerPtr numBufferNode = nodeMap.GetNode("StreamBufferCountManual");

    // We cannot provide more buffers than allowed though.
    int bufferCount = min(maxBuffers, _bufferCount);
    Log("Setting number of camera buffers to " + to_string(bufferCount), Frames);
    numBufferNode->SetValue(bufferCount);
    Log("Number of camera buffers set to " + to_string(bufferCount), Frames);

    _isConnected = true;
    _connectedSerialNumber = serialNumber;
    *camera = connectedCamera;
    Log("Camera connected", Frames);
    return true;
}

void FlirCamera::WaitForConnected(bool attemptToConnect)
{
    Log("Waiting for camera to connect", Frames);
    while(!_isConnected)
    {
        if(attemptToConnect && TryConnect(_connectedSerialNumber, &_camera))
        {
            break;
        }
        
        usleep(100000);
    }
    Log("Camera connected", Frames);
}

void FlirCamera::EnsureConnectionNotLost()
{
    if(_shouldBeConnected)
    {
        WaitForConnected();
    }
}

void FlirCamera::PowerCycle()
{
    Log("Power cycling camera", Frames);
    //Log("DeInit camera", Frames);
    //_camera->DeInit();
    Log("Resetting camera device", Frames);
    _camera->DeviceReset();

    // Now we need to wait for the camerato be connected again.
    WaitForConnected(true);

    // Apply the settings that were previously set.
    Log("Applying original camera user settings", Frames);
    if(_userFrameWidth)
    {
        SetFrameWidth(*_userFrameWidth);
    }

    if(_userFrameHeight)
    {
        SetFrameHeight(*_userFrameHeight);
    }

    if(_userFrameRate)
    {
        SetFrameRate(*_userFrameRate);
    }

    if(_userFilter)
    {
        SetFilter(*_userFilter);
    }

    Log("Camera power cycle finished", Frames);
}
