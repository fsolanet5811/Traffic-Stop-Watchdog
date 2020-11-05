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
        void Close();
    private:
        int _port;
        mutex _portKey;
    };

    enum Device
    {
        Handheld = 0,
        Motors = 1
    };

    struct DeviceMessage
    {
        Device device;
        vector<uchar> bytes;
    };

    class DeviceSerialPort
    {
    public:
        DeviceSerialPort(SerialPort& port);
        void StartGathering();
        void StopGathering();
        bool IsGathering();
        DeviceMessage ReadFromDevice(Device device);
        void WriteToDevice(Device device, vector<uchar> data);
        void WriteToDevice(vector<uchar> formattedData);
        void WriteToDevice(uchar formattedByte);

    private:
        bool _isGathering;
        future<void> _gatherFuture;
        SerialPort* _port;
        vector<DeviceMessage> _buffer;
        mutex _bufferKey;
        void Gather();
    };

    class CameraMotionController
    {
    public:
        CameraMotionController(FlirCamera& camera, OfficerLocator& officerLocator, DeviceSerialPort& commandPort);
        double VerticalFov;
        double HorizontalFov;
        uint CameraFramesToSkip;
        void StartCameraMotionGuidance();
        void StopCameraMotionGuidance();
        bool IsGuidingCameraMotion();

    private:
        FlirCamera* _camera;
        OfficerLocator* _officerLocator;
        DeviceSerialPort* _commandPort;
        bool _isGuidingCameraMotion;
        uint _cameraLivefeedCallbackKey;
        void OnLivefeedImageReceived(LiveFeedCallbackArgs args);
        static int AngleToMotorValue(double angle);
    };

    enum CommandAction
    {
        StartOfficerTracking = 0,
        StopOfficerTracking = 1,
        SendKeyword = 2,
        RelativeMoveSynchronous = 3,
        RelativeMoveAsynchronous = 4,
        AbsoluteMoveSynchronous = 5,
        AbsoluteMoveAsynchronous = 6,
    };

    struct Command
    {
        CommandAction action;
        vector<unsigned char> args;
    };

    class CommandAgent
    {
    public:
        CommandAgent(DeviceSerialPort& commandPort);
        Command* ReadCommand(Device device);
        vector<unsigned char> ReadResponse(Device device);
        void SendCommand(Device device, Command* command);
        void AcknowledgeReceived(Device device);

    private:
        DeviceSerialPort* _commandPort;
    };

    class Settings
    {
    public:
        Settings();
        Settings(string settingsFile);
        string DeviceSerialPath;
        string CameraSerialNumber;
        short OfficerClassId;
        void Load(string settingsFile);
    };
}