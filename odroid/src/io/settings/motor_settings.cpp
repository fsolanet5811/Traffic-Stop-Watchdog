#include "settings.hpp"

using namespace tsw::io::settings;

MotorSettings::MotorSettings() { }

MotorSettings::MotorSettings(string settingsFile)
{
    Load(settingsFile);
}

void MotorSettings::Load(string settingsFile)
{
    Document doc;
    Settings::LoadJsonFile(&doc, settingsFile);

    MotorsSerialPath = doc["MotorsSerialPath"].GetString();
    PanConfig = ReadMotorConfig(doc, "PanConfig");
    TiltConfig = ReadMotorConfig(doc, "TiltConfig");
    LogFlags = ReadLogFlags(doc, "LogFlags");
}

