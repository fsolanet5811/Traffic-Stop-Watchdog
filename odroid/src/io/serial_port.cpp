#include "io.hpp"
#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

using namespace tsw::io;
using namespace std;

SerialPort::SerialPort(speed_t baudRate)
{
    _port = -1;
    _baudRate = baudRate;
}

void SerialPort::Open(string devicePath)
{
    // Attempt to connect to the device.
    Log("Opening serial port on " + devicePath, RawSerialContinuous);
    int res = open(devicePath.c_str(), O_RDWR);

    // See if we made a successful connection.
    if(res < 0)
    {
        throw runtime_error("Could not connect to device " + devicePath);
    }
    Log("Found port on " + to_string(res), RawSerialContinuous);
    _port = res;

    // Now configure the connection.
    // First we gotta load in the current settings.
    termios tty;
    res = tcgetattr(_port, &tty);

    // Make sure the read worked.
    if(res != 0)
    {
        _port = -1;
        throw runtime_error("Could not read device attributes.");
    }

    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size 
    tty.c_cflag |= CS8; // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    tty.c_cc[VTIME] = 10;    // Wait for up to .1s (1 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    // Set in/out baud rate to be 115200
    cfsetispeed(&tty, _baudRate);
    cfsetospeed(&tty, _baudRate);

    // Save our current settings modifications.
    if(tcsetattr(_port, TCSANOW, &tty) != 0)
    {
        _port = -1;
        throw runtime_error("Could not save device attributes.");
    }

    // Ok now we are good.
    Log("Port opened.", RawSerialContinuous);
}

int SerialPort::Read(unsigned char* buffer, int bytesToRead)
{
    Log("Trying to read " + to_string(bytesToRead) + " bytes.", RawSerialContinuous);
    int bytesRead = read(_port, buffer, bytesToRead);

    // Make sure it worked.
    if(bytesRead == -1)
    {
        throw runtime_error("Failed to read bytes.");
    }

    // Only inform the raw serial if we actually read something.
    uint logFlags = bytesRead > 0 ? RawSerial | RawSerialContinuous : RawSerialContinuous;
	Log("Read " + to_string(bytesRead) + " bytes. (" + ToHex(buffer, bytesRead) + ")", logFlags);

    return bytesRead;
}

int SerialPort::Write(unsigned char* data, int bytesToWrite)
{
    Log("Trying to write " + to_string(bytesToWrite) + " bytes (" + ToHex(data, bytesToWrite) + ")", RawSerialContinuous | RawSerial);
    int bytesWritten = write(_port, data, bytesToWrite);

    if(bytesWritten == -1)
    {
        throw runtime_error("Failed to write bytes.");
    }

    Log("Wrote " + to_string(bytesWritten) + " bytes.", RawSerialContinuous | RawSerial);
    return bytesWritten;
}

void SerialPort::Clear()
{
    uchar temp[4096];
    while(Read(temp, 4096)) { }
}

void SerialPort::Close()
{
    close(_port);
}

SerialPort::~SerialPort()
{
    Close();
}

string SerialPort::ToHex(uchar* bytes, int numBytes)
{
    stringstream ss;
    int i;
    for(i = 0; i < numBytes - 1; i++)
    {
        ss << "0x";
        ss << setw(2) << setfill('0') << hex << static_cast<uint>(bytes[i]);
        ss << ' ';
    }

    if(numBytes > 0)
    {
        ss << "0x";
        ss << setw(2) << setfill('0') << hex << static_cast<uint>(bytes[i]);
    }
    
    return ss.str();
}