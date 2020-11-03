#include "io.hpp"
#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

using namespace tsw::io;
using namespace std;

SerialPort::SerialPort()
{
    _port = -1;
}

void SerialPort::Open(string devicePath)
{
    // Attempt to connect to the device.
    int res = open(devicePath.c_str(), O_RDWR);
    
    // See if we made a successful connection.
    if(res == -1)
    {
        throw runtime_error("Could not connect to device " + devicePath);
    }

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

    tty.c_cc[VTIME] = 1;    // Wait for up to .1s (1 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    // Set in/out baud rate to be 9600
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    // Save our current settings modifications.
    if(tcsetattr(_port, TCSANOW, &tty) != 0)
    {
        _port = -1;
        throw runtime_error("Could not save device attributes.");
    }

    // Ok now we are good.
}

int SerialPort::Read(unsigned char* buffer, int bytesToRead)
{
    _portKey.lock();
    int bytesRead = read(_port, buffer, bytesToRead);
    _portKey.unlock();

    // Make sure it worked.
    if(bytesToRead == -1)
    {
        throw runtime_error("Failed to read bytes.");
    }

    return bytesRead;
}

int SerialPort::Write(unsigned char* data, int bytesToWrite)
{
    _portKey.lock();
    int bytesWritten = write(_port, data, bytesToWrite);
    _portKey.unlock();

    if(bytesWritten == -1)
    {
        throw runtime_error("Failed to write bytes.");
    }

    return bytesWritten;
}