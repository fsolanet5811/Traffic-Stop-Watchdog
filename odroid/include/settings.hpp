#pragma once

#include "rapidjson.h"
#include "document.h"
#include "io.hpp"

using namespace std;
using namespace tsw::io;

namespace tsw::io::settings
{
    class Settings
    {
    public:
        uint LogFlags;

    protected:
        static void LoadJsonFile(Document* doc, string jsonFile);
        static MotorConfig ReadMotorConfig(Document& doc, string motorConfigName);
        static Vector2 ReadVector2(Document& doc, string vectorName);
        static Bounds ReadBounds(Document& doc, string boundsName);
        static uint ReadLogFlags(Document& doc, string logFlagsName);

    private:
        static bool ReadLogFlag(Document& doc, string logFlagsName, string flagName);

    };

    class TswSettings : public Settings
    {
    public:
        TswSettings();
        TswSettings(string settingsFile);
        string DeviceSerialPath;
        string MotorsSerialPath;
        string HandheldSerialPath;
        string CameraSerialNumber;
        bool UseDeviceAdapter;
        short OfficerClassId;
        Vector2 TargetRegionProportion;
        Vector2 SafeRegionProportion;
        int CameraFramesToSkipMoving;
	Vector2 HomeAngles;
        double CameraFrameRate;
        int CameraFrameWidth;
        int CameraFrameHeight;
        MotorConfig PanConfig;
        MotorConfig TiltConfig;
        void Load(string settingsFile);

    private:
        static bool ReadLogFlag(Document& doc, string flagName);
    };

    class MotorSettings : public Settings
    {
    public:
        MotorSettings();
        MotorSettings(string settingsFile);
        string MotorsSerialPath;
        MotorConfig PanConfig;
        MotorConfig TiltConfig;
        void Load(string settingsFile);
    };
}
