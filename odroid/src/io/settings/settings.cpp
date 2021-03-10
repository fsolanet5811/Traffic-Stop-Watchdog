#include "settings.hpp"
#include <rapidjson.h>
#include <document.h>
#include "filereadstream.h"

using namespace tsw::io::settings;

void Settings::LoadJsonFile(Document* doc, string jsonFile)
{
    FILE* settingsFileHandle = fopen(jsonFile.c_str(), "r");
    char readBuffer[65536];
    FileReadStream settingsStream(settingsFileHandle, readBuffer, sizeof(readBuffer));

    // Read in the file.
    doc->ParseStream(settingsStream);
    fclose(settingsFileHandle);
}

Vector2 Settings::ReadVector2(Document& doc, string vectorName)
{
    Vector2 v;
    v.x = doc[vectorName.c_str()]["X"].GetDouble();
    v.y = doc[vectorName.c_str()]["Y"].GetDouble();
    return v;
}

MotorConfig Settings::ReadMotorConfig(Document& doc, string motorConfigName)
{
    MotorConfig mc;
    mc.angleBounds.max = doc[motorConfigName.c_str()]["AngleBounds"]["Max"].GetInt();
    mc.angleBounds.min = doc[motorConfigName.c_str()]["AngleBounds"]["Min"].GetInt();
    mc.stepBounds.max = doc[motorConfigName.c_str()]["StepBounds"]["Max"].GetInt();
    mc.stepBounds.min = doc[motorConfigName.c_str()]["StepBounds"]["Min"].GetInt();
    return mc;
}

Bounds Settings::ReadBounds(Document& doc, string boundsName)
{
    Bounds b;
    b.max = doc[boundsName.c_str()]["Max"].GetInt();
    b.min = doc[boundsName.c_str()]["Min"].GetInt();
}

uint Settings::ReadLogFlags(Document& doc, string logFlagsName)
{
    uint logFlags = 0;
    logFlags |= ReadLogFlag(doc, logFlagsName, "Error");
    logFlags |= ReadLogFlag(doc, logFlagsName, "Debug") << 1;
    logFlags |= ReadLogFlag(doc, logFlagsName, "Information") << 2;
    logFlags |= ReadLogFlag(doc, logFlagsName, "Frames") << 3;
    logFlags |= ReadLogFlag(doc, logFlagsName, "Officers") << 4;
    logFlags |= ReadLogFlag(doc, logFlagsName, "Movements") << 5;
    logFlags |= ReadLogFlag(doc, logFlagsName, "Recording") << 6;
    logFlags |= ReadLogFlag(doc, logFlagsName, "RawSerial") << 7;
    logFlags |= ReadLogFlag(doc, logFlagsName, "DeviceSerial") << 8;
    logFlags |= ReadLogFlag(doc, logFlagsName, "Acknowledge") << 9;
    return logFlags;
}

SerialConfig Settings::ReadSerialConfig(Document& doc, string serialConfigName)
{
    SerialConfig sc;
    sc.path = doc[serialConfigName.c_str()]["Path"].GetString();
    sc.baudRate = ParseBaudRate(doc[serialConfigName.c_str()]["BaudRate"].GetInt());
    return sc;
}

bool Settings::ReadLogFlag(Document& doc, string logFlagsName, string flagName)
{
    return doc[logFlagsName.c_str()][flagName.c_str()].GetBool();
}

speed_t Settings::ParseBaudRate(int baudRate)
{
    switch(baudRate)
    {
        case 9600:
            return B9600;

        case 115200:
            return B115200;

        default:
            throw runtime_error("Unimplemented baud rate: " + to_string(baudRate));
    }
}