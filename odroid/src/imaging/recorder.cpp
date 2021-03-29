#include "imaging.hpp"
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include "opencv2/opencv.hpp"
#include "io.hpp"
#include <functional>

using namespace tsw::imaging;
using namespace tsw::io;
using namespace Spinnaker;
using namespace cv;
using namespace std;

Recorder::Recorder(FlirCamera& camera)
{
    _camera = &camera;
	_isRecording = false;
    _frameBufferLock.Name = "REC";
}

void Recorder::StartRecording(string fileName)
{
    // If we are already recording, do nothing.
    if(!_isRecording)
    {
        _isRecording = true;
        _recordedFileName = fileName;
       
        // Configure the video.
        Size frameSize;
        frameSize.height = _camera->GetFrameHeight();
        frameSize.width = _camera->GetFrameWidth();
        int cc = VideoWriter::fourcc('M', 'J', 'P', 'G');
        if(!_aviWriter.open(_recordedFileName, cc, _camera->GetFrameRate(), frameSize))
        {
            throw runtime_error("Video could not be initialized.");
        }

        // Listen for when the camera gets frames.
        _callbackKey = _camera->RegisterLiveFeedCallback(bind(&Recorder::OnLiveFeedImageReceived, this, placeholders::_1));

        // Start the thread that actually saves these frames.
        _recordFuture = async(launch::async, [this]()
        {
            Record();
        });
    }
}

void Recorder::StopRecording()
{
    if(IsRecording())
    {
        _isRecording = false;

        // Wait for the recording thread to finish. (This could take a while!)
        _recordFuture.wait();

        // Stop listening for the new frames.
        _camera->UnregisterLiveFeedCallback(_callbackKey);

        // Close the video up.
        _aviWriter.release();
    }
}

bool Recorder::IsRecording()
{
    return _isRecording;
}

void Recorder::OnLiveFeedImageReceived(LiveFeedCallbackArgs args)
{
    // In case this gets invoked after we stop recording.
    if(IsRecording())
    {
        // Add this image frame to our buffer.
        // Another thread will take care of actually recording it.
        // We do this so that images can be acquired as fast as possible.
        Log("Adding frame # " + to_string(args.imageIndex) + " to recording buffer", Recording);
        _frameBufferLock.Lock("Add Image");
        _frameBuffer.push(args.image);
        _frameBufferLock.Unlock("Add Image");
        Log("Frame added to recording buffer", Recording);
    }
}

void Recorder::Record()
{
	size_t frameIndex = 0;
    while(IsRecording())
    {
        // Put all of the frames that are in the buffer in the video.
        while(true)
        {
            _frameBufferLock.Lock("Record");
            size_t framesLeft = _frameBuffer.size();
            Log(to_string(framesLeft) + " frames found in buffer", Recording);
            if(framesLeft <= 0)
            {
                // No frames to record.
                _frameBufferLock.Unlock("Record");
                break;
            }

            // Access and remove the next frame.
            ImagePtr image = _frameBuffer.front();
            _frameBuffer.pop();
            _frameBufferLock.Unlock("Record");

            // Put this frame in the video.
            _aviWriter.write(MatFromImage(image));
            Log("Frame " + to_string(frameIndex++) + " recorded", Recording);
        }

        // Wait a bit before recording more frames.
        usleep(100000);
    }
}

Mat Recorder::MatFromImage(ImagePtr image)
{
    // Put the pointer into an array of bytes.
    unsigned char* data = (unsigned char*)image->GetData();
    vector<unsigned char> imageBytes(data, data + image->GetImageSize());

    Mat m(image->GetHeight(), image->GetWidth(), CV_8UC3, data);
    
    // Opencv interperets the data as bgr instead of rgb, so we gotta convert it.
    cvtColor(m, m, COLOR_BGR2RGB);

    return m; 
}
