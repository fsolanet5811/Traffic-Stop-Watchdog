#pragma once

#include <string>
#include <termios.h>

using namespace std;

namespace tsw::common
{
    struct ImageProcessingConfig
    {
        bool displayFrames;
        bool recordFrames;
        bool showBoxes;
        bool moveCamera;
    };

    struct OfficerInferenceBox
    {
        short topLeftX;
        short topLeftY;
        short bottomRightX;
        short bottomRightY;
        float confidence;
    };

    struct Vector2
    {
        double x;
        double y;
    };

    struct OfficerDirection
    {
        bool foundOfficer;
        bool shouldMove;
        Vector2 movement;
    };

    struct ByteVector2
    {
        unsigned char x;
        unsigned char y;
    };

        struct Bounds
    {
        int min;
        int max;
    };

    struct MotorConfig
    {
        Bounds angleBounds;
        Bounds stepBounds;
    };

    struct SerialConfig
    {
        string path;
        speed_t baudRate;
    };
}