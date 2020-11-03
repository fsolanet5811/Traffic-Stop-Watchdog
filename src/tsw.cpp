#include "imaging.hpp"
#include "io.hpp"
#include "Spinnaker.h"

using namespace tsw::imaging;

void OnLiveFeedImageReceived(LiveFeedCallbackArgs args)
{

}

int main()
{
    string serialNumber = "20386745";
    
    FlirCamera camera;
    camera.Connect(serialNumber);

    cout << camera.GetDeviceTemperature() << endl;
    cout << to_string(camera.GetFrameRate()) << endl;
    camera.SetFrameRate(30);

    //camera.RegisterLiveFeedCallback(OnLiveFeedImageReceived);
    //camera.UnregisterLiveFeedCallback(OnLiveFeedImageReceived);

    Recorder rec(camera);
    camera.StartLiveFeed();

    rec.StartRecording("my_video");

    this_thread::sleep_for(chrono::seconds(5));
    rec.StopRecording();
    camera.StopLiveFeed();

    return 0;
}