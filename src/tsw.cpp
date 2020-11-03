#include "imaging.hpp"
#include "io.hpp"
#include "Spinnaker.h"

using namespace tsw::imaging;

void OnLiveFeedImageReceived(LiveFeedCallbackArgs args)
{

}

int main()
{
    FlirCamera camera;
    string serial = "20386745";
    cout << "Connecting to camera " << serial << endl;
    camera.Connect(serial);
    cout << "Connected" << endl;

    Recorder r(camera);
    r.StartRecording("test.avi");
    camera.StartLiveFeed();

    sleep(5);

    r.StopRecording();
    camera.StopLiveFeed();
    
    return 0;
    
    
}