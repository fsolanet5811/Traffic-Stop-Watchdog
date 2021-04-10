#include "imaging.hpp"
#include "settings.hpp"
#include <functional>

using namespace tsw::imaging;
using namespace tsw::io::settings;

ImageProcessor::ImageProcessor(Recorder& recorder, DisplayWindow& window, FlirCamera& camera, OfficerLocator& officerLocator, CameraMotionController& motionController, ImageProcessingConfig config)
{
    _recorder = &recorder;
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
        if(_config.recordFrames || _config.displayFrames || _config.moveCamera)
        {
            _livefeedCallbackKey = _camera->RegisterLiveFeedCallback(bind(&ImageProcessor::OnLiveFeedImageReceived, this, placeholders::_1));

            if(_config.recordFrames)
            {
                _recorder->StartRecording("1_OfficerFootage.avi");
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
        if(_config.recordFrames || _config.displayFrames || _config.moveCamera)
        {
            _camera->UnregisterLiveFeedCallback(_livefeedCallbackKey);

            if(_config.recordFrames)
            {
                _recorder->StopRecording();
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

    if(_config.recordFrames || _config.displayFrames)
    {
        // Make an opencv image out of the FLIR image.
        Mat cvImage = MatFromImage(args.image);

        // If there is a box and we want to record and/or display the image, we gotta draw the box.
        if(bestBox)
        {
            // Create the rectange in cv language.
            Point topLeft(bestBox->topLeftX, bestBox->topLeftY);
            Point bottomRight(bestBox->bottomRightX, bestBox->bottomRightY);
            
            // This is what puts it on the mat.
            rectangle(cvImage, topLeft, bottomRight, Scalar(255, 0, 0));
        }

        if(_config.recordFrames)
        {
            Log("Adding frame # " + to_string(args.imageIndex) + " to recording buffer", Recording);
            _recorder->AddFrame(cvImage.clone());
            Log("Frame added to recording buffer", Recording);
        }

        if(_config.displayFrames)
        {
            _window->Update(cvImage);
        }
    }
}

bool ImageProcessor::IsProcessing()
{
    return _isProcessing;
}

Mat ImageProcessor::MatFromImage(ImagePtr image)
{
    // Put the pointer into an array of bytes.
    unsigned char* data = (unsigned char*)image->GetData();
    Mat m(image->GetHeight(), image->GetWidth(), CV_8UC3, data);

    // Opencv interperets the data as bgr instead of rgb, so we gotta convert it.
    cvtColor(m, m, COLOR_BGR2RGB);

    //Scalar min(85, 76, 127);
    //Scalar max(124, 255, 255);

    //inRange(m, min, max, m);

    if(_config.showBoxes)
    {
        // Draw the highest confidence bounding box.
        vector<OfficerInferenceBox> boxRes = _officerLocator->GetOfficerLocations(image);
        if(boxRes.size() > 0)
        {
            OfficerInferenceBox bestBox = boxRes.at(0);
            for(int i = 1; i < boxRes.size(); i++)
            {
                OfficerInferenceBox curBox = boxRes[i];
                if(curBox.confidence > bestBox.confidence);
                {
                    bestBox = curBox;
                }
            }

            // Create the rectange in cv language.
            Point topLeft(bestBox.topLeftX, bestBox.topLeftY);
            Point bottomRight(bestBox.bottomRightX, bestBox.bottomRightY);
            
            // This is what puts it on the mat.
            rectangle(m, topLeft, bottomRight, Scalar(255, 0, 0));
        }
    }

    return m; 
}