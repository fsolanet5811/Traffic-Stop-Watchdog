#include "io.hpp"

using namespace tsw::io;

CommandAgent::CommandAgent(SerialPort& commandPort)
{
    _commandPort = &commandPort;
}

Command* CommandAgent::ReadCommand()
{
    // The first byte contains all of the info we need to read the command.
    unsigned char* commandInfo = new unsigned char[1];
}