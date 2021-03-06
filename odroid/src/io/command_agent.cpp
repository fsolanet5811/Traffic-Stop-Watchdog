#include "io.hpp"
#include "utilities.hpp"
#include <string>

using namespace tsw::io;
using namespace tsw::utilities;

CommandAgent::CommandAgent(DeviceSerialPort& commandPort)
{
    _commandPort = &commandPort;
}

Command* CommandAgent::ReadCommand(Device device)
{
    Log("Reading command from " + to_string(device), DeviceSerial);

    // Grab a message from the device serial port.
    DeviceMessage message = _commandPort->ReadFromDevice(device);

    // Bits 0-3 from the first byte will tell us what the command is.
    CommandAction action = (CommandAction)(message.bytes[0] & 0x0f);

    // The rest of the bytes are the actual arguments.
    message.bytes.erase(message.bytes.begin());
    Command* c = new Command();
    c->action = action;
    c->args = message.bytes;

    Log("Command read from " + to_string(device), DeviceSerial);

    return c;
}

void CommandAgent::SendCommand(Device device, Command* command)
{
    Log("Sending Command to " + to_string(device), DeviceSerial);
    // We gotta construct the command byte from the action, device, and number of bytes.
    // They better have no more than 7 bytes in their command!
    if(command->args.size() > 7)
    {
        throw runtime_error("Command cannot contain more than 7 argument bytes");
    }

    _commandPort->WriteToDevice(device, command->action, command->args);
    Log(string("Command sent to " + to_string(device)) + ". Waiting for acknowledge", DeviceSerial);

    // Wait for the acknowledgement.
    _commandPort->ReadFromDevice(device);
    Log("Acknowledge received", DeviceSerial);
}

void CommandAgent::AcknowledgeReceived(Device device)
{
    // An acknowledgement is just 4 ones as the lsb's.
    _commandPort->WriteToDevice(device, Acknowledge);
}

bool CommandAgent::TryReadResponse(Device device, vector<unsigned char>* readResponse)
{
    DeviceMessage message;
    if(_commandPort->TryReadFromDevice(device, &message))
    {
        /// We read a response. Give them a reference to the bytes.
        *readResponse = message.bytes;
        return true;
    }

    // No response found.
    return false;
}

vector<unsigned char> CommandAgent::ReadResponse(Device device)
{
    Log("Reading response from " + device, DeviceSerial);
    vector<unsigned char> response;
    while(true)
    {
        if(TryReadResponse(device, &response))
        {
            // A response was read. Return it.
            Log("Response read from " + device, DeviceSerial);
            return response;
        }

        // Sleep a bit before trying again.
        usleep(100000);
    }
}

CommandAgent::~CommandAgent()
{
    delete _commandPort;
}