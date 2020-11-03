#include "spinnaker/SpinGenApi/SpinnakerGenApi.h"
#include "spinnaker/Spinnaker.h"
#include "imaging.hpp"
#include <iostream>
#include <sstream>
#include <future>
#include <thread>
#include <chrono>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace tsw::imaging;
using namespace std;

FlirCamera::FlirCamera()
{
    _system = System::GetInstance();  
    _nextLiveFeedKey = 1;
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
    // Grab hold of the list of cameras from the system.
    _system->UpdateCameras();
    CameraList cameras = _system->GetCameras();
    
    // This will grab the camera that has the given serial number.
    _camera = cameras.GetBySerial(serialNumber);
    if(!_camera)
    {
        throw runtime_error("Could not find camera with serial number: " + serialNumber + ".");
    }

    _camera->Init();

    // These are settings that we want on the camera always.
    _camera->ExposureAuto = ExposureAuto_Continuous;
    _camera->GainAuto = GainAuto_Continuous;
    _camera->AcquisitionMode = AcquisitionMode_Continuous;
}

double FlirCamera::GetDeviceTemperature()
{
    return _camera->DeviceTemperature.GetValue();
}

double FlirCamera::GetFrameRate()
{
    return _camera->AcquisitionResultingFrameRate.GetValue();
}

void FlirCamera::SetFrameRate(double hertz)
{
    _camera->AcquisitionFrameRateEnable = true;
    _camera->AcquisitionFrameRate = hertz;
}

void FlirCamera::StartLiveFeed()
{
    // This ensures that we don't have multiple livefeed instances started.
    if(!IsLiveFeedOn())
    {
        _isLiveFeedOn = true;
        _liveFeedFuture = async(launch::async, [this]()
        {
            RunLiveFeed();
        });
    }
}

void FlirCamera::StopLiveFeed()
{
    _isLiveFeedOn = false;

    // This will wait for the live feed thread to stop.
    _liveFeedFuture.wait();
}

bool FlirCamera::IsLiveFeedOn()
{
    return _isLiveFeedOn;
}

uint FlirCamera::RegisterLiveFeedCallback(function<void(LiveFeedCallbackArgs)> callback)
{
    _liveFeedKey.lock();

    // This will create a key entry for this callback.
    LiveFeedCallback cb;
    cb.callback = callback;
    cb.callbackKey = _nextLiveFeedKey++;
    _liveFeedCallbacks.push_back(cb);
    _liveFeedKey.unlock();
    
    // Give them back the key so they can remove the callback later.
    return cb.callbackKey;
}

void FlirCamera::UnregisterLiveFeedCallback(uint callbackKey)
{
    _liveFeedKey.lock();
    _liveFeedCallbacks.remove_if([callbackKey](LiveFeedCallback cb)
    {
        return cb.callbackKey == callbackKey;
    });
    _liveFeedKey.unlock();
}

void FlirCamera::OnLiveFeedImageReceived(ImagePtr image, uint imageIndex)
{
    LiveFeedCallbackArgs args;
    args.image = image;
    args.imageIndex = imageIndex;

    // We do not want to call any of the callbacks while one of them is being removed/added.
    _liveFeedKey.lock();
    for(LiveFeedCallback cb : _liveFeedCallbacks)
    {
        cb.callback(args);
    }

    // Now we can mess with the callback list.
    _liveFeedKey.unlock();
}

void FlirCamera::RunLiveFeed()
{
    // Begin acquireung camera frames.
    _camera->BeginAcquisition();

    // This will keep track of which frame we are on.
    uint imageIndex = 0;
    while(IsLiveFeedOn())
    {
        // Grab an image from the camera
        ImagePtr image = _camera->GetNextImage();
        ChunkData data = image->GetChunkData();
        
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