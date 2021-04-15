#include "utilities.hpp"
#include <iostream>
#include <iomanip>
#include <ctime>

using namespace tsw::utilities;

uint _logFlags = tsw::utilities::Information;
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
        time_t t = std::time(nullptr);
        tm lt = *localtime(&t);
        cout << put_time(&lt, "%m-%d-%Y %H:%M:%S") << " | " << s << endl;
    }
    _logKey.unlock();
}
