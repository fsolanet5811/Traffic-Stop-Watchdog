#include "imaging.hpp"
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include "opencv.hpp"
#include <functional>

using namespace tsw::imaging;
using namespace Spinnaker;
using namespace cv;
using namespace std;

Recorder::Recorder(FlirCamera& camera)
{
    _camera = &camera;
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
    }
}

void Recorder::StopRecording()
{
    // Stop listening for the new frames.
    _camera->UnregisterLiveFeedCallback(_callbackKey);

    // Close the video up.
    _aviWriter.release();
}

bool Recorder::IsRecording()
{
    return _isRecording;
}

void Recorder::OnLiveFeedImageReceived(LiveFeedCallbackArgs args)
{
    // Add this image frame to our video.
    _aviWriter.write(MatFromImage(args.image));
    cout << "Frame Recorded" << endl;
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