#include "imaging.hpp"

using namespace tsw::imaging;

int main()
{
    FlirCamera camera;
    string serial = "20386745";
    cout << "Connecting to camera " << serial << endl;
    camera.Connect(serial);
    cout << "Connected" << endl;
    
    cout << "FPS: " << camera.GetFrameRate();

    cout << "Creating recorder" << endl;

    Recorder r(camera);

    cout << "Starting record" << endl;
    r.StartRecording("1test.avi");

    cout << "Starting live feed" << endl;
    camera.StartLiveFeed();

    cout << "Waiting 3 seconds" << endl;
    sleep(3);

    cout << "Stopping live feed" << endl;
    camera.StopLiveFeed();

    cout << "Stopping recording" << endl;
    r.StopRecording();
}