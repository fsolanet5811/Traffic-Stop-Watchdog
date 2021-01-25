#include "io.hpp"

using namespace tsw::io;
using namespace std;

int main(int argc, char* argv[])
{
	SerialPort port;
	port.Open(argv[1]);
	
	int val1 = stoi(argv[2]);
	int val2 = stoi(argv[3]);
	

	uchar* michaelsBulshitByte = new uchar[1];
	michaelsBulshitByte[0] = 0x30;
	int sent = port.Write(michaelsBulshitByte, 1);
	cout << "Sent: " << sent << endl;

	uchar* data = new uchar[7];
	data[0] = 0x64;
	data[1] = val1 & 0x0f00 >> 16;
	data[2] = val1 & 0x00f0 >> 8;
	data[3] = val1 & 0x000f;
	data[4] = val2 & 0x0f00 >> 16;
	data[5] = val2 & 0x0f00 >> 8;
	data[6] = val2 & 0x0f00;
	sent = port.Write(data, 7);
	cout << "Sent: " << sent << endl;
}
