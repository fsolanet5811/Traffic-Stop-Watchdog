#include "imaging.hpp"
#include <functional>

using namespace tsw::imaging;
using namespace Spinnaker;

DisplayWindow::DisplayWindow(string windowName, int refreshRate)
{
    WindowName = windowName;
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
        _showFuture.wait();
    }
    
}

void DisplayWindow::RunWindow()
{
    destroyAllWindows();
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
    destroyAllWindows();
}

void DisplayWindow::Update(Mat currentFrame)
{
    // Not sure if we need a lock here, but better safe than sorry.
    _displayLock.Lock("Assigning new frame");
    _currentFrame = currentFrame;
    _displayLock.Unlock("Assigning new frame");
}
