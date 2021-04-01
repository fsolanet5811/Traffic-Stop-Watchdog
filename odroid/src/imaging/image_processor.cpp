#include "imaging.hpp"
#include "settings.hpp"
#include <functional>

using namespace tsw::imaging;
using namespace tsw::io::settings;

ImageProcessor::ImageProcessor(Recorder& recorder, DisplayWindow& window, FlirCamera& camera, OfficerLocator& officerLocator, ImageProcessingConfig config)
{
    _recorder = &recorder;
    _window = &window;
    _camera = &camera;
    _officerLocator = &officerLocator;
    _config = config;
    _isProcessing = false;
}

void ImageProcessor::StartProcessing()
{
    if(!IsProcessing())
    {
        if(_config.recordFrames || _config.displayFrames)
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
        }
        
        _isProcessing = true;
    }
    
}

void ImageProcessor::StopProcessing()
{
    if(IsProcessing())
    {
        if(_config.recordFrames || _config.displayFrames)
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
        }
        _camera->UnregisterLiveFeedCallback(_livefeedCallbackKey);
        _isProcessing = false;
    }
}

void ImageProcessor::OnLiveFeedImageReceived(LiveFeedCallbackArgs args)
{
    Mat cvImage = MatFromImage(args.image);

    if(_config.recordFrames)
    {
        Log("Adding frame # " + to_string(args.imageIndex) + " to recording buffer", Recording);
        _recorder->AddFrame(cvImage.clone());
        Log("Frame added to recording buffer", Recording);
    }

    if(_config.displayFrames)
    {
        _window->Update(cvImage.clone());
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
    vector<unsigned char> imageBytes(data, data + image->GetImageSize());

    Mat m(image->GetHeight(), image->GetWidth(), CV_8UC3, data);


    // Opencv interperets the data as bgr instead of rgb, so we gotta convert it.
    cvtColor(m, m, COLOR_BGR2RGB);

    if(_config.showBoxes)
    {
        // Draw the highest confidence bounding box.
        vector<InferenceBoundingBox> boxRes = _officerLocator->GetOfficerLocations(image);
        if(boxRes.size() > 0)
        {
            InferenceBoundingBox bestBox = boxRes.at(0);
            for(int i = 1; i < boxRes.size(); i++)
            {
                InferenceBoundingBox curBox = boxRes.at(i);
                if(curBox.confidence > bestBox.confidence);
                {
                    bestBox = curBox;
                }
            }

            // Create the rectange in cv language.
            Point topLeft(bestBox.rect.topLeftXCoord, bestBox.rect.topLeftYCoord);
            Point bottomRight(bestBox.rect.bottomRightXCoord, bestBox.rect.bottomRightYCoord);
            
            // This is what puts it on the mat.
            rectangle(m, topLeft, bottomRight, Scalar(255, 0, 0));
        }
    }

    return m; 
}