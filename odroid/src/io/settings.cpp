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
    MotorsSerialPath = doc["MotorsSerialPath"].GetString();
    HandheldSerialPath = doc["HandheldSerialPath"].GetString();
    CameraSerialNumber = doc["CameraSerialNumber"].GetString();
    UseDeviceAdapter = doc["UseDeviceAdapter"].GetBool();
    OfficerClassId = doc["OfficerClassId"].GetInt();
    TargetRegionProportion = ReadVector2(doc, "TargetRegionProportion");
    SafeRegionProportion = ReadVector2(doc, "SafeRegionProportion");
    CameraFramesToSkipMoving = doc["CameraFramesToSkipMoving"].GetInt();
    CameraFrameRate = doc["CameraFrameRate"].GetDouble();
    CameraFrameHeight = doc["CameraFrameHeight"].GetInt();
    CameraFrameWidth = doc["CameraFrameWidth"].GetInt();
    PanConfig = ReadMotorConfig(doc, "PanConfig");
    TiltConfig = ReadMotorConfig(doc, "TiltConfig");

    // Get the log settings.
    LogFlags = 0;
    LogFlags |= ReadLogFlag(doc, "Error");
    LogFlags |= ReadLogFlag(doc, "Debug") << 1;
    LogFlags |= ReadLogFlag(doc, "Information") << 2;
    LogFlags |= ReadLogFlag(doc, "Frames") << 3;
    LogFlags |= ReadLogFlag(doc, "Officers") << 4;
    LogFlags |= ReadLogFlag(doc, "Movements") << 5;
    LogFlags |= ReadLogFlag(doc, "Recording") << 6;
    LogFlags |= ReadLogFlag(doc, "RawSerial") << 7;
    LogFlags |= ReadLogFlag(doc, "DeviceSerial") << 8;
    LogFlags |= ReadLogFlag(doc, "Acknowledge") << 9;
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

Bounds Settings::ReadBounds(Document& doc, string boundsName)
{
    Bounds b;
    b.max = doc[boundsName.c_str()]["Max"].GetInt();
    b.min = doc[boundsName.c_str()]["Min"].GetInt();
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