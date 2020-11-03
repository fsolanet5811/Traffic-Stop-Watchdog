#include "imaging.hpp"

using namespace tsw::imaging;
using namespace std;

int main()
{
    FlirCamera camera;

    while(true)
    {
        cout << "Scanning available cameras..." << endl;

        vector<string> serials = camera.FindDevices();

        cout << "Found " << serials.size() << " cameras." << endl;

        for(int i = 0; i < serials.size(); i++)
        {
            cout << serials[i] << endl;
        }
        
        sleep(2);
    }

    return 0;
}