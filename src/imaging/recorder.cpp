#include "imaging.hpp"
#include "SpinVideo.h"
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <functional>

using namespace tsw::imaging;
using namespace Spinnaker;
using namespace Spinnaker::Video;

Recorder::Recorder(FlirCamera& camera)
{
    _camera = &camera;
}

void Recorder::StartRecording(string fileName)
{
    // If we are already recording, do nothing;
    if(!_isRecording)
    {
        _isRecording = true;
        _recordedFileName = fileName;

        // Configure the video.
        AVIOption option;
        option.frameRate = _camera->GetFrameRate();
        _recordedVideo.SetMaximumFileSize(0);
        _recordedVideo.Open(_recordedFileName.c_str(), option);

        // Listen for when the camera gets frames.
        _callbackKey = _camera->RegisterLiveFeedCallback(bind(&Recorder::OnLiveFeedImageReceived, this, placeholders::_1));
    }
}

void Recorder::StopRecording()
{
    // Stop listening for the new frames.
    _camera->UnregisterLiveFeedCallback(_callbackKey);

    // Close the video up.
    _recordedVideo.Close(); 
}

bool Recorder::IsRecording()
{
    return _isRecording;
}

void Recorder::OnLiveFeedImageReceived(LiveFeedCallbackArgs args)
{
    // Add this image frame to our video.
    _recordedVideo.Append(args.image);
}