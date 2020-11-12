#include "io.hpp"
#include <string>
#include <fstream>
#include <streambuf>
#include <rapidjson.h>
#include <document.h>
#include "filereadstream.h"

using namespace tsw::io;
using namespace std;
using namespace rapidjson;

Settings::Settings()
{
    CameraFramesToSkipMoving = 0;
}

Settings::Settings(string settingsFile)
{
    Load(settingsFile);
    
}

void Settings::Load(string settingsFile)
{
    FILE* settingsFileHandle = fopen(settingsFile.c_str(), "r");
    char readBuffer[65536];
    FileReadStream settingsStream(settingsFileHandle, readBuffer, sizeof(readBuffer));
    Document doc;

    // Read in the file.
    doc.ParseStream(settingsStream);
    fclose(settingsFileHandle);
    
    // Now we can populate our settings.
    DeviceSerialPath = doc["DeviceSerialPath"].GetString();
    CameraSerialNumber = doc["CameraSerialNumber"].GetString();
    OfficerClassId = doc["OfficerClassId"].GetInt();
    TargetRegionProportion = ReadVector2(doc, "TargetRegionProportion");
    SafeRegionProportion = ReadVector2(doc, "SafeRegionProportion");
    CameraFramesToSkipMoving = doc["CameraFramesToSkipMoving"].GetInt();
    CameraFrameRate = doc["CameraFrameRate"].GetDouble();
    CameraFrameHeight = doc["CameraFrameHeight"].GetInt();
    CameraFrameWidth = doc["CameraFrameWidth"].GetInt();

    // Get the log settings.
    LogFlags |= ReadLogFlag(doc, "Error");
    LogFlags |= ReadLogFlag(doc, "Debug") << 1;
    LogFlags |= ReadLogFlag(doc, "Info") << 2;

    // Yes, the skip is intentional. I was too lazy to shift everything around, so deal with it.
    LogFlags |= ReadLogFlag(doc, "Frames") << 4;
    LogFlags |= ReadLogFlag(doc, "Officers") << 5;
    LogFlags |= ReadLogFlag(doc, "Movements") << 6;
    LogFlags |= ReadLogFlag(doc, "Recording") << 7;
    LogFlags |= ReadLogFlag(doc, "RawSerial") << 8;
    LogFlags |= ReadLogFlag(doc, "DeviceSerial") << 9;
    LogFlags |= ReadLogFlag(doc, "Acknowledge") << 10;
}

Vector2 Settings::ReadVector2(Document& doc, string vectorName)
{
    Vector2 v;
    v.x = doc[vectorName.c_str()]["X"].GetDouble();
    v.y = doc[vectorName.c_str()]["Y"].GetDouble();
    return v;
}

bool Settings::ReadLogFlag(Document& doc, string flagName)
{
    return doc["LogFlags"][flagName.c_str()].GetBool();
}