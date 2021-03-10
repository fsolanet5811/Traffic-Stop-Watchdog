#include "io.hpp"

using namespace tsw::io;

uint _logFlags = tsw::io::Information;
mutex _logKey;

void ConfigureLog(uint flags)
{
    _logKey.lock();
    _logFlags = flags;
    _logKey.unlock();
}

// This will log something to the console if the flags associated with it match us.
void Log(string s, uint flags)
{
    _logKey.lock();
    if(flags & _logFlags)
    {
        cout << s << endl;
    }
    _logKey.unlock();
}
