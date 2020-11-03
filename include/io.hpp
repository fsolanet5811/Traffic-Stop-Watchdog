#pragma once

#include "Spinnaker.h"
#include "imaging.hpp"
#include <future>

using namespace Spinnaker;
using namespace tsw::imaging;

namespace tsw::io
{
    class SerialPort
    {
    public:
        SerialPort();
        void Open(string devicePath);
        int Read(unsigned char* buffer, int bytesToRead);
        int Write(unsigned char* data, int bytesToWrite);

    private:
        int _port;
        mutex _portKey;
    };

    enum Device
    {
        Handheld = 0,
        Motors = 1
    };

    struct DeviceByte
    {
        Device device;
        unsigned char value;
    };

    class DeviceSerialPort
    {
    public:
        DeviceSerialPort(SerialPort& port);
        void StartGathering();
        void StopGathering();
        bool IsGathering();
        vector<unsigned char> ReadFromDevice(Device device, int numBytes);
        void WriteToDevice(Device device, vector<unsigned char> data);

    private:
        bool _isGathering;
        future<void> _gatherFuture;
        SerialPort* _port;
        vector<DeviceByte> _buffer;
        mutex _bufferKey;
        void Gather();
    };

    class CameraMotionController
    {
    public:
        CameraMotionController(FlirCamera& camera, OfficerLocator& officerLocator);
        double VerticalFov;
        double HorizontalFov;
        uint CameraFramesToSkip;
        void StartCameraMotionGuidance();
        void StopCameraMotionGuidance();
        bool IsGuidingCameraMotion();

    private:
        FlirCamera* _camera;
        OfficerLocator* _officerLocator;
        bool _isGuidingCameraMotion;
        uint _cameraLivefeedCallbackKey;
        void OnLivefeedImageReceived(LiveFeedCallbackArgs args);
    };

    enum CommandAction
    {
        StartOfficerTracking = 0,
        StopOfficerTracking = 1,
        SendKeyword = 2
    };

    struct Command
    {
        CommandAction action;
        unsigned char* argBytes;
        int numArgBytes;
    };

    class CommandAgent
    {
    public:
        CommandAgent(SerialPort& commandPort);
        Command* ReadCommand();
        static void DeleteCommand(Command* command);

    private:
        SerialPort* _commandPort;
    };

    class Settings
    {
    public:
        string DeviceSerialPath;
        void Load(string settingsFile);
    };
}