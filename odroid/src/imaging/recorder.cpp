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

Recorder::Recorder(Size frameSize, double fps)
{
    _frameSize = frameSize;
    _fps = fps;
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
        int cc = VideoWriter::fourcc('M', 'J', 'P', 'G');
        if(!_aviWriter.open(_recordedFileName, cc, _fps, _frameSize))
        {
            throw runtime_error("Video could not be initialized.");
        }

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

        // Close the video up.
        _aviWriter.release();
    }
}

bool Recorder::IsRecording()
{
    return _isRecording;
}

void Recorder::AddFrame(Mat frame)
{
    // In case this gets invoked after we stop recording.
    if(IsRecording())
    {
        // Add this image frame to our buffer.
        // Another thread will take care of actually recording it.
        // We do this so that images can be acquired as fast as possible.
        _frameBufferLock.Lock("Add Image");
        _frameBuffer.push(frame);
        _frameBufferLock.Unlock("Add Image");
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
            Mat image = _frameBuffer.front();
            _frameBuffer.pop();
            _frameBufferLock.Unlock("Record");

            // Put this frame in the video.
            _aviWriter.write(image);
            Log("Frame " + to_string(frameIndex++) + " recorded", Recording);
        }

        // Wait a bit before recording more frames.
        usleep(100000);
    }
}
