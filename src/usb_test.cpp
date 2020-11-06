#include "io.hpp"

using namespace tsw::io;

int main(int argc, char* argv[])
{
    SerialPort port;
    port.Open(argv[1]);

    uchar* buffer = new uchar[1];
    for(int i = 0; i < 10; i++)
    {
        cout << port.Read(buffer, 1) << " ";
        cout << buffer[0] << endl;
    }

    delete buffer;
}