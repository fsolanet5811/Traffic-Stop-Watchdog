#include "io.hpp"
#include <string>
#include <fstream>
#include <streambuf>
#include <json.h>

using namespace tsw::io;
using namespace std;
using namespace Json;

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
    ifstream settignsStream;
    settignsStream.open(settingsFile);
    CharReaderBuilder builder;
    Value root;
    JSONCPP_STRING errors;

    // Read in the file.
    if(!parseFromStream(builder, settignsStream, &root, &errors))
    {
        // Something did not work.
        throw runtime_error("Failed to read settings. " + errors);
    }

    // Now we can populate our settings.
    DeviceSerialPath = root["DeviceSerialPath"].asString();
    CameraSerialNumber = root["CameraSerialNumber"].asString();
    OfficerClassId = root["OfficerClassId"].asInt();
    TargetRegionProportion = ReadVector2(root["TargetRegionProportion"]);
    SafeRegionProportion = ReadVector2(root["SafeRegionProportion"]);
    CameraFramesToSkipMoving = root["CameraFramesToSkipMoving"].asInt();
}

Vector2 Settings::ReadVector2(Value jsonVector)
{
    Vector2 v;
    v.x = jsonVector["X"].asDouble();
    v.y = jsonVector["Y"].asDouble();
    return v;
}