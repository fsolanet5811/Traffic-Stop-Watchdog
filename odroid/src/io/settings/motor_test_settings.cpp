#include "settings.hpp"

using namespace tsw::io::settings;

MotorTestSettings::MotorTestSettings() { }

MotorTestSettings::MotorTestSettings(string settingsFile)
{
    Load(settingsFile);
}

void MotorTestSettings::Load(string settingsFile)
{
    Document doc;
    Settings::LoadJsonFile(&doc, settingsFile);

    MotorsSerialPath = doc["MotorsSerialPath"].GetString();
    PanConfig = ReadMotorConfig(doc, "PanConfig");
    TiltConfig = ReadMotorConfig(doc, "TiltConfig");
    LogFlags = ReadLogFlags(doc, "LogFlags");
}

