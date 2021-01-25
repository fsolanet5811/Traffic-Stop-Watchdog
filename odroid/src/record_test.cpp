#include "imaging.hpp"

using namespace tsw::imaging;

int main(int argc, char* argv[])
{
    FlirCamera camera;
    string serial = "20386745";
    cout << "Connecting to camera " << serial << endl;
    camera.Connect(serial);
    cout << "Connected" << endl;
    camera.SetFrameRate(25);
    camera.SetFrameWidth(1080);
    camera.SetFrameHeight(720);
    cout << "FPS: " << camera.GetFrameRate() << endl;
    cout << "Size: " << camera.GetFrameWidth() << " X " << camera.GetFrameHeight() << endl;

    cout << "Creating recorder" << endl;

    Recorder r(camera);

    cout << "Starting record" << endl;
    r.StartRecording("1test.avi");

    cout << "Starting live feed" << endl;
    camera.StartLiveFeed();

    cout << "Waiting 5 seconds" << endl;
    sleep(5);

    cout << "Stopping live feed" << endl;
    camera.StopLiveFeed();

    cout << "Stopping recording" << endl;
    r.StopRecording();
}