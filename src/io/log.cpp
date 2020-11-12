#include "io.hpp"

using namespace tsw::io;

uint LOG_FLAGS = tsw::io::Information;

void ConfigureLog(uint flags)
{
    LOG_FLAGS = flags;
}

// This will log something to the console if the flags associated with it match us.
void Log(string s, uint flags)
{
    if(flags & LOG_FLAGS)
    {
        cout << s << endl;
    }
}
