#include "imaging.hpp"

using namespace tsw::imaging;
using namespace std;

int main()
{
    FlirCamera camera;
    string serial = "20386745";
    cout << "Connecting to camera " << serial << endl;
    camera.Connect(serial);
    cout << "Connected" << endl;
}