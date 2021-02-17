#include "io.hpp"

using namespace tsw::io;

MultiPortCommandAgent::MultiPortCommandAgent(DeviceSerialPort& handheldPort, DeviceSerialPort& motorsPort) : CommandAgent(handheldPort)
{
    _handheldPort = &handheldPort;
    _motorsPort = &motorsPort;
}

Command* MultiPortCommandAgent::ReadCommand(Device device)
{
    _commandPort = GetDeviceSerialPort(device);
    return CommandAgent::ReadCommand(device);
}

void MultiPortCommandAgent::SendCommand(Device device, Command* command)
{
    _commandPort = GetDeviceSerialPort(device);
    CommandAgent::SendCommand(device, command);
}

void MultiPortCommandAgent::AcknowledgeReceived(Device device)
{
    _commandPort = GetDeviceSerialPort(device);
    CommandAgent::AcknowledgeReceived(device);
}

void MultiPortCommandAgent::SendResponse(vector<uchar> formattedResponse)
{
    // The device should just be the first bit in the first char.
    Device device = (Device)(formattedResponse[0] >> 7);
    _commandPort = GetDeviceSerialPort(device);
    CommandAgent::SendResponse(formattedResponse);
}

bool MultiPortCommandAgent::TryReadResponse(Device device, vector<uchar>* readResponse)
{
    _commandPort = GetDeviceSerialPort(device);
    return TryReadResponse(device, readResponse);
}

DeviceSerialPort* MultiPortCommandAgent::GetDeviceSerialPort(Device device)
{
    if(device == Motors)
    {
        return _motorsPort;
    }

    if(device == Handheld)
    {
        return _handheldPort;
    }

    throw runtime_error("Could not get device serial port for device " + to_string(device));
}

MultiPortCommandAgent::~MultiPortCommandAgent()
{
    delete _handheldPort;
    delete _motorsPort;
}