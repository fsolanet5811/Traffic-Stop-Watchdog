#include "io.hpp"
#include <future>
#include <thread>
#include <chrono>

using namespace tsw::io;
using namespace std;

DeviceSerialPort::DeviceSerialPort(SerialPort& port)
{
    _port = &port;
}

bool DeviceSerialPort::IsGathering()
{
    return _isGathering;
}

void DeviceSerialPort::StartGathering()
{
    if(!IsGathering())
    {
        _isGathering = true;
        _gatherFuture = async(launch::async, [this]()
        {
            Gather();
        });
    }
}

void DeviceSerialPort::Gather()
{
    while(IsGathering())
    {
        // Get a byte from the port.
        unsigned char b[1];
        int read = _port->Read(b, 1);

        // See if we read anything.
        if(read)
        {
            // Determine where this byte came from.
            Device dev = (Device)(b[0] >> 7);

            // Add this byte to the list.
            DeviceByte db;
            db.value = b[0];
            db.device = dev;
            _bufferKey.lock();
            _buffer.push_back(db);
            _bufferKey.unlock();

            // We are not sleeping here because if we have nothing to read, then the read will act as a small sleep.
            // Plus, if we slep after every byte, it will take a while to read things.
        }
    }
}

vector<unsigned char> DeviceSerialPort::ReadFromDevice(Device device, int numBytes)
{
    // The goal is to block until we get all the bytes we need.
    vector<unsigned char> bytes;
    
    while(bytes.size() < numBytes)
    {
        // Get a hold of the list of bytes.
        _bufferKey.lock();
        for(int i = 0; i < _buffer.size() && bytes.size() < numBytes; i++)
        {
            // Only grab the byte if it is from the device we want.
            if(_buffer[i].device == device)
            {
                // Put this byte in our list and remove it from the overall buffer.
                bytes.push_back(_buffer[i].value);
                _buffer.erase(_buffer.begin() + i--);
            }
        }

        // Let other people use the list.
        _bufferKey.unlock();

        // Wait a bit before reading again, but only if we aren't full.
        if(bytes.size() < numBytes)
        {
            usleep(100000);
        }
    }

    // At this point, we have read all of the bytes and can return them.
    return bytes;
}

void DeviceSerialPort::WriteToDevice(Device device, vector<unsigned char> data)
{
    // We have to put the device as the MSB.
    unsigned char* dataWithDevice = new unsigned char[data.size()];
    for(int i = 0; i < data.size(); i++)
    {
        unsigned char dataByte = (unsigned char)device << 7;
        dataByte |= data[i] & 0x80;
        dataWithDevice[i] = dataByte;
    }

    // Now the bytes are ready to send to through serial to the device.
    _port->Write(dataWithDevice, data.size());
}