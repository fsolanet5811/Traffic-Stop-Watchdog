#include "io.hpp"

using namespace tsw::io;

CommandAgent::CommandAgent(DeviceSerialPort& commandPort)
{
    _commandPort = &commandPort;
}

Command* CommandAgent::ReadCommand(Device device)
{
    // Grab a message from the device serial port.
    DeviceMessage message = _commandPort->ReadFromDevice(device);

    // Bits 0-3 from the first byte will tell us what the command is.
    CommandAction action = (CommandAction)(message.bytes[0] & 0x0f);

    // The rest of the bytes are the actual arguments.
    message.bytes.erase(message.bytes.begin());
    Command* c = new Command();
    c->action = action;
    c->args = message.bytes;

    return c;
}

void CommandAgent::SendCommand(Device device, Command* command)
{
    // We gotta construct the command byte from the action, device, and number of bytes.
    // They better have no more than 7 bytes in their command!
    if(command->args.size() > 7)
    {
        throw runtime_error("Command cannot contain more than 7 argument bytes");
    }

    uchar cByte = ((uchar)device << 7) | (command->args.size() << 4) | command->action;

    vector<uchar> bytes = command->args;
    bytes.insert(bytes.begin(), cByte);

    _commandPort->WriteToDevice(device, bytes);

    // Wait for the acknowledgement.
    _commandPort->ReadFromDevice(device);
}

void CommandAgent::AcknowledgeReceived(Device device)
{
    // An acknowledgement is just 4 ones as the lsb's.
    uchar ack = ((uchar)device << 7) | 0x0f;
    _commandPort->WriteToDevice(ack);
}