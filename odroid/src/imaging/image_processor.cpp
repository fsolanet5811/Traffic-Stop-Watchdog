#include "imaging.hpp"
#include "settings.hpp"
#include <functional>

using namespace tsw::imaging;
using namespace tsw::io::settings;

ImageProcessor::ImageProcessor(DisplayWindow& window, FlirCamera& camera, SmartOfficerLocator& officerLocator, CameraMotionController& motionController, ImageProcessingConfig config)
{
    // We have two recorders. One for the footage, and one for the filter.
    Size frameSize(camera.GetFrameWidth(), camera.GetFrameHeight());
    double fps = camera.GetFrameRate();
    _footageRecorder = new Recorder(frameSize, fps);
    _filterRecorder = new Recorder(frameSize, fps);

    _window = &window;
    _camera = &camera;
    _officerLocator = &officerLocator;
    _motionController = &motionController;
    _config = config;
    _isProcessing = false;
}

void ImageProcessor::StartProcessing()
{
    if(!IsProcessing())
    {
        if(_config.recordFrames || _config.displayFrames || _config.moveCamera || _config.recordFilter)
        {
            _livefeedCallbackKey = _camera->RegisterLiveFeedCallback(bind(&ImageProcessor::OnLiveFeedImageReceived, this, placeholders::_1));

            if(_config.recordFrames)
            {
                _footageRecorder->StartRecording("1_OfficerFootage.avi");
            }

            if(_config.recordFilter)
            {
                _filterRecorder->StartRecording("1_OfficerFilter.avi");
            }

            if(_config.displayFrames)
            {
                _window->Show();
            }

            if(_config.moveCamera)
            {
                _motionController->InitializeGuidance();
            }
        }
        
        _isProcessing = true;
    }
    
}

void ImageProcessor::StopProcessing()
{
    if(IsProcessing())
    {
        if(_config.recordFrames || _config.displayFrames || _config.moveCamera || _config.recordFilter)
        {
            _camera->UnregisterLiveFeedCallback(_livefeedCallbackKey);

            if(_config.recordFrames)
            {
                _footageRecorder->StopRecording();
            }

            if(_config.recordFilter)
            {
                _filterRecorder->StopRecording();
            }

            if(_config.displayFrames)
            {
                _window->Close();
            }

            if(_config.moveCamera)
            {
                _motionController->UninitializeGuidance();
            }
        }
        _camera->UnregisterLiveFeedCallback(_livefeedCallbackKey);
        _isProcessing = false;
    }
}

void ImageProcessor::OnLiveFeedImageReceived(LiveFeedCallbackArgs args)
{
    // Find the desired bounding box on the oficer.
    OfficerInferenceBox* bestBox = _officerLocator->GetOfficerBox(args.image);

    // Do the motion first, since that is the only time sensitive thing really.
    if(_config.moveCamera && args.imageIndex % CameraFramesToSkip == 0)
    {
        // Based on the best box, see where we need to go.
        OfficerDirection dir = _officerLocator->FindOfficer(args.image, bestBox);

        _motionController->GuideCameraTo(dir);
    }

    if(_config.recordFrames || _config.displayFrames || _config.recordFilter)
    {
        // Make an opencv image out of the FLIR image.
        Mat cvImage = MatFromImage(args.image, bestBox);

        if(_config.recordFrames || _config.displayFrames)
        {
            Mat footageFrame = cvImage.clone();
            DrawOfficerBox(bestBox, footageFrame, Scalar(255, 0, 0));

            if(_config.recordFrames)
            {
                Log("Adding frame # " + to_string(args.imageIndex) + " to footage recording buffer", Recording);
                _footageRecorder->AddFrame(footageFrame);
                Log("Frame added to footage recording buffer", Recording);
            }

            if(_config.displayFrames)
            {
                _window->Update(footageFrame.clone());
            }
        }

        if(_config.recordFilter)
        {
            Log("Adding frame # " + to_string(args.imageIndex) + " to filter recording buffer", Recording);
            Mat filtered;
            cvtColor(cvImage, filtered, COLOR_BGR2HSV);
            Mat threshold;
            inRange(filtered, _officerLocator->MinHSV, _officerLocator->MaxHSV, threshold);
            Mat filteredColor;
            cvtColor(threshold, filteredColor, COLOR_GRAY2RGB);
            DrawOfficerBox(bestBox, filteredColor, Scalar(255, 50, 50));
            _filterRecorder->AddFrame(filteredColor);
            Log("Frame added to filter recording buffer", Recording);
        }
    }
}

bool ImageProcessor::IsProcessing()
{
    return _isProcessing;
}

void ImageProcessor::DrawOfficerBox(OfficerInferenceBox* officerBox, Mat cvImage, Scalar color)
{
    // If there is a box and we want to record and/or display the image, we gotta draw the box.
    if(_config.showBoxes && officerBox)
    {
        // Create the rectange in cv language.
        Point topLeft(officerBox->topLeftX, officerBox->topLeftY);
        Point bottomRight(officerBox->bottomRightX, officerBox->bottomRightY);
        
        // This is what puts it on the mat.
        rectangle(cvImage, topLeft, bottomRight, color);
    }
}

Mat ImageProcessor::MatFromImage(ImagePtr image, OfficerInferenceBox* officerBox)
{
    // Put the pointer into an array of bytes.
    unsigned char* data = (unsigned char*)image->GetData();
    Mat m(image->GetHeight(), image->GetWidth(), CV_8UC3, data);

    // Opencv interperets the data as bgr instead of rgb, so we gotta convert it.
    cvtColor(m, m, COLOR_BGR2RGB);
    return m; 
}

ImageProcessor::~ImageProcessor()
{
    delete _footageRecorder;
    delete _filterRecorder;
}