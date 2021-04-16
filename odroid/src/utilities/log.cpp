#include "utilities.hpp"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <sys/time.h>

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
        // This is a lot of bs to calculate the current time in microseconds.
        timeval tAbs;
        gettimeofday(&tAbs, nullptr);
        tm lt = *localtime(&tAbs.tv_sec);
        stringstream usecondsStr;
        usecondsStr << setfill('0') << setw(6) << tAbs.tv_usec;

        cout << put_time(&lt, "%m-%d-%Y %H:%M:%S.") << usecondsStr.str() << " | " << s << endl;
    }
    _logKey.unlock();
}
