#include "settings.hpp"
#include <rapidjson.h>
#include <document.h>
#include "filereadstream.h"

using namespace tsw::io::settings;
using namespace cv;

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

ByteVector2 Settings::ReadByteVector2(Document& doc, string vectorName)
{
    ByteVector2 v;
    v.x = doc[vectorName.c_str()]["X"].GetInt();
    v.y = doc[vectorName.c_str()]["Y"].GetInt();
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
    return b;
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
    logFlags |= ReadLogFlag(doc, logFlagsName, "RawSerialContinuous") << 7;
    logFlags |= ReadLogFlag(doc, logFlagsName, "DeviceSerial") << 8;
    logFlags |= ReadLogFlag(doc, logFlagsName, "Acknowledge") << 9;
    logFlags |= ReadLogFlag(doc, logFlagsName, "Locking") << 10;
    logFlags |= ReadLogFlag(doc, logFlagsName, "Flir") << 11;
    logFlags |= ReadLogFlag(doc, logFlagsName, "RawSerial") << 12;
    logFlags |= ReadLogFlag(doc, logFlagsName, "LED") << 13;
    logFlags |= ReadLogFlag(doc, logFlagsName, "OpenCV") << 14;
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

ImageProcessingConfig Settings::ReadImageProcessingConfig(Document& doc, string imageProcessingConfigName)
{
    ImageProcessingConfig config;
    config.displayFrames = doc[imageProcessingConfigName.c_str()]["DisplayFrames"].GetBool();
    config.recordFrames = doc[imageProcessingConfigName.c_str()]["RecordFrames"].GetBool();
    config.showBoxes = doc[imageProcessingConfigName.c_str()]["ShowBoxes"].GetBool();
    config.moveCamera = doc[imageProcessingConfigName.c_str()]["MoveCamera"].GetBool();

    return config;
}

Scalar Settings::ReadHSV(Document& doc, string hsvName)
{
    Scalar hsv;
    hsv[0] = doc[hsvName.c_str()]["H"].GetDouble();
    hsv[1] = doc[hsvName.c_str()]["S"].GetDouble();
    hsv[2] = doc[hsvName.c_str()]["V"].GetDouble();
    return hsv;
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
