#include "io.hpp"
#include <string>
#include <fstream>
#include <streambuf>
#include <rapidjson.h>
#include <document.h>
#include "filereadstream.h"
#include "settings.hpp"

using namespace tsw::io;
using namespace tsw::io::settings;
using namespace std;
using namespace rapidjson;

TswSettings::TswSettings()
{
    CameraFramesToSkipMoving = 0;
}

TswSettings::TswSettings(string settingsFile)
{
    Load(settingsFile);   
}

void TswSettings::Load(string settingsFile)
{  
    // Read in the file.
    Document doc;
    LoadJsonFile(&doc, settingsFile);
    
    // Now we can populate our settings.
    DeviceSerialPath = doc["DeviceSerialPath"].GetString();
    MotorsSerialPath = doc["MotorsSerialPath"].GetString();
    HandheldSerialPath = doc["HandheldSerialPath"].GetString();
    CameraSerialNumber = doc["CameraSerialNumber"].GetString();
    UseDeviceAdapter = doc["UseDeviceAdapter"].GetBool();
    OfficerClassId = doc["OfficerClassId"].GetInt();
    TargetRegionProportion = ReadVector2(doc, "TargetRegionProportion");
    HomeAngles = ReadVector2(doc, "HomeAngles");
    SafeRegionProportion = ReadVector2(doc, "SafeRegionProportion");
    CameraFramesToSkipMoving = doc["CameraFramesToSkipMoving"].GetInt();
    CameraFrameRate = doc["CameraFrameRate"].GetDouble();
    CameraFrameHeight = doc["CameraFrameHeight"].GetInt();
    CameraFrameWidth = doc["CameraFrameWidth"].GetInt();
    PanConfig = ReadMotorConfig(doc, "PanConfig");
    TiltConfig = ReadMotorConfig(doc, "TiltConfig");

    // Get the log settings.
    LogFlags = ReadLogFlags(doc, "LogFlags");
}






