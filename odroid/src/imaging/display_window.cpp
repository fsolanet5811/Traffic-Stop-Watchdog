#include "imaging.hpp"
#include <functional>

using namespace tsw::imaging;

DisplayWindow::DisplayWindow(FlirCamera& camera, string windowName, int refreshRate)
{
    WindowName = windowName;
    _liveFeedCallbackKey = 0;
    _camera = &camera;
    _isShown = false;
    _refreshRate = refreshRate;
    _displayLock.Name = "DSP";
}

bool DisplayWindow::IsShown()
{
    return _isShown;
}

void DisplayWindow::Show()
{
    if(!IsShown())
    {
        _isShown = true;
        _liveFeedCallbackKey = _camera->RegisterLiveFeedCallback(bind(&DisplayWindow::OnLiveFeedImageReceived, this, placeholders::_1));
        _showFuture = async(launch::async, [this]()
        {
            RunWindow();
        });
    }
}

void DisplayWindow::Close()
{
    if(IsShown())
    {
        _isShown = false;
        _camera->UnregisterLiveFeedCallback(_liveFeedCallbackKey);
        _showFuture.wait();
    }
    
}

void DisplayWindow::RunWindow()
{
    int i = 0;
    namedWindow(WindowName, WINDOW_FULLSCREEN);
    while(IsShown())
    {
        if(_currentFrame.rows > 0 && _currentFrame.cols > 0)
        {
            _displayLock.Lock("Showing frame");
            imshow(WindowName, _currentFrame);
            _displayLock.Unlock("Showing frame");
        }

        // Wait a bit before updating again.
        // This also actually lets opencv keep the window up.
        waitKey(1000 / _refreshRate);
    }
    destroyWindow(WindowName);
}

void DisplayWindow::OnLiveFeedImageReceived(LiveFeedCallbackArgs args)
{
    _displayLock.Lock("Assigning new frame");
    _currentFrame = MatFromImage(args.image);
    _displayLock.Unlock("Assigning new frame");
}

Mat DisplayWindow::MatFromImage(ImagePtr image)
{
    // Put the pointer into an array of bytes.
    unsigned char* data = (unsigned char*)image->GetData();
    vector<unsigned char> imageBytes(data, data + image->GetImageSize());

    Mat m(image->GetHeight(), image->GetWidth(), CV_8UC3, data);
    
    // Opencv interperets the data as bgr instead of rgb, so we gotta convert it.
    cvtColor(m, m, COLOR_BGR2RGB);

    return m; 
}