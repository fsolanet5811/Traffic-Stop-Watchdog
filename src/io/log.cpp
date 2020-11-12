#include "io.hpp"

using namespace tsw::io;

// These are what we currently will log to the console.
uint _flags = Information;

void ConfigureLog(uint flags)
{
    _flags = flags | 0x01;
}

void Log(string s, uint flags)
{
    if(flags & _flags)
    {
        cout << s << endl;
    }
}
