#include "io.hpp"
#include "utilities.hpp"
#include <future>
#include <thread>
#include <chrono>


using namespace tsw::io;
using namespace std;
using namespace tsw::utilities;

DeviceSerialPort::DeviceSerialPort(SerialPort& port)
{
    _port = &port;
    _bufferLock.Name = "DSP";
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

void DeviceSerialPort::StopGathering()
{
    if(IsGathering())
    {
        _isGathering = false;
        _gatherFuture.wait();
    }
}

void DeviceSerialPort::Gather()
{
    // We need to keep track of which device we are reading from.
    Device currentDevice;
    int bytesForCurrent = 0;
    vector<unsigned char> currentMessage;

    while(IsGathering())
    {
        // Get a byte from the port.
        unsigned char b[1];
        int read = _port->Read(b, 1);

        // See if we read anything.
        if(read)
        {
            // Determine where this byte came from.
            if(!bytesForCurrent)
            {
                currentDevice = (Device)(b[0] >> 7);

                // Bits 4-6 will tell us how many extra bytes are coming from this device.
                bytesForCurrent = 1 + ((b[0] & 0b01110000) >> 4);
            }
            
            // Add this byte to the list.
            Log("Read byte from " + to_string((int)currentDevice) + ": " + SerialPort::ToHex(b, 1), DeviceSerial);
            currentMessage.push_back(b[0]);
            bytesForCurrent--;
            
            // See if we reached the end of the message.
            if(!bytesForCurrent)
            {
                // Add this message to the buffer.
                DeviceMessage message;
                message.device = currentDevice;
                message.bytes = currentMessage;

                _bufferLock.Lock("Add Message");
                _buffer.push_back(message);
                _bufferLock.Unlock("Add Message");

                currentMessage = vector<unsigned char>();
            }
            
            // We are not sleeping here because if we have nothing to read, then the read will act as a small sleep.
            // Plus, if we slep after every byte, it will take a while to read things.
        }
    }
}

DeviceMessage DeviceSerialPort::ReadFromDevice(Device device)
{
    // The goal is to block until we get the message from the device we need.
    DeviceMessage message;
    while(true)
    {
        // Try to read from the device.
        if(TryReadFromDevice(device, &message))
        {
            // We successfully read.
            return message;
        }

        // Wait a bit before reading again.
        usleep(10000);
    }
}

DeviceMessage DeviceSerialPort::ReadFromDevice(Device device, unsigned char header)
{
    // Keep reading messages until one with the given header is returned.
    while(true)
    {
        DeviceMessage message = ReadFromDevice(device);
        if(message.bytes[0] == header)
        {
            return message;
        }
    }
}

void DeviceSerialPort::WriteToDevice(vector<unsigned char> formattedData)
{
    Log("Writing " + to_string(formattedData.size()) + " bytes to serial", RawSerialContinuous);
    _port->Write(formattedData.data(), formattedData.size());
}

void DeviceSerialPort::WriteToDevice(Device device, CommandAction command, vector<unsigned char> data)
{
    // Because we only have 3 bits for extra byte count, we cannot have more than 7 bytes.
    if(data.size() > 7)
    {
        throw runtime_error("Cannot send more than 7 bytes as arguments to device.");
    }

    // Create the header byte and put it in the beginning.
    unsigned char header = ((unsigned char)device << 7) | data.size() << 4 | command;
    data.insert(data.begin(), header);
    WriteToDevice(data);
}

void DeviceSerialPort::WriteToDevice(Device device, CommandAction action, unsigned char data)
{
    vector<unsigned char> dataVec(1);
    dataVec[0] = data;
    WriteToDevice(device, action, dataVec);
}

void DeviceSerialPort::WriteToDevice(Device device, CommandAction action)
{
    vector<unsigned char> bytes(0);
    WriteToDevice(device, action, bytes);
}

bool DeviceSerialPort::TryReadFromDevice(Device device, DeviceMessage* readMessage)
{
    // Get a hold of the list of bytes.
    _bufferLock.Lock("Read Message");
    for(int i = 0; i < _buffer.size(); i++)
    {
        // Only grab the byte if it is from the device we want.
        if(_buffer[i].device == device)
        {
            // We can return this message once we remove it from the list and unlock the buffer.
            DeviceMessage found = _buffer[i];
            _buffer.erase(_buffer.begin() + i);
            _bufferLock.Unlock("Read Message");
            *readMessage = found;
            return true;
        }
    }

    // Release the list.
    _bufferLock.Unlock("Read Message");
    return false;
}

DeviceSerialPort::~DeviceSerialPort()
{
    if(IsGathering())
    {
        StopGathering();
    }
    delete _port;
}
