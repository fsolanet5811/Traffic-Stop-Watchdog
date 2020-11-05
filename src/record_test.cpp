#include "imaging.hpp"

using namespace tsw::imaging;

int main(int argc, char* argv[])
{
    string thisFile(argv[0]);
    string startingDir = thisFile.substr(0, startingDir.find_last_of('/'));

    FlirCamera camera;
    string serial = "20386745";
    cout << "Connecting to camera " << serial << endl;
    camera.Connect(serial);
    cout << "Connected" << endl;
    camera.SetFrameRate(25);
    camera.SetFrameWidth(1080);
    camera.SetFrameWidth(720);
    cout << "FPS: " << camera.GetFrameRate() << endl;
    cout << "Size: " << camera.GetFrameWidth() << " X " << camera.GetFrameHeight() << endl;

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