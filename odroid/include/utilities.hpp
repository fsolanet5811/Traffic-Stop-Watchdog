#pragma once

#include <string>
#include <future>

using namespace std;

namespace tsw::utilities
{
    class SmartLock
    {
    public:
        string Name;
        SmartLock();
        SmartLock(string name);
        void Lock(string description);
        void Unlock(string description);

    private:
        mutex _lock;
        string BuildDescriptor(string description);
    };

    enum LogFlag
    {
        Error = 0b1,
        Debug = 0b10,
        Information = 0b100,
        Frames = 0b1000,
        Officers = 0b10000,
        Movements = 0b100000,
        Recording = 0b1000000,
        RawSerialContinuous = 0b10000000,
        DeviceSerial = 0b100000000,
        Acknowledge = 0b1000000000,
        Locking = 0b10000000000,
        Flir = 0b100000000000,
        RawSerial = 0b1000000000000,
        LED = 0b10000000000000,
        OpenCV = 0b100000000000000
    };
}

void ConfigureLog(uint flags);
void Log(string s, uint flags);
