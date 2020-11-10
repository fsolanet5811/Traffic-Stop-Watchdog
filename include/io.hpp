#pragma once

#include "Spinnaker.h"
#include "imaging.hpp"
#include "rapidjson.h"
#include "document.h"
#include <future>

using namespace Spinnaker;
using namespace tsw::imaging;
using namespace std;
using namespace rapidjson;

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
        bool TryReadFromDevice(Device device, DeviceMessage* readMessage);

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
        Vector2 HomeAngles;
        uint CameraFramesToSkip;
        void StartCameraMotionGuidance();
        void StopCameraMotionGuidance();
        bool IsGuidingCameraMotion();
        void OfficerSearch();
        void GoToHome();
        void SendSyncRelativeMoveCommand(double horizontal, double vertical);
        void SendAsyncRelativeMoveCommand(double horizontal, double vertical);
        void SendSyncAbsoluteMoveCommand(double horizontal, double vertical);
        void SendAsyncAbsoluteMoveCommand(double horizontal, double vertical);
        static int GetMaxValue();

    private:
        enum OfficerSearchState
        {
            NotSearching = 0,
            CheckingLastSeen = 1,
            MovingToHomePosition = 2,
            Circling = 3
        };
        FlirCamera* _camera;
        OfficerLocator* _officerLocator;
        DeviceSerialPort* _commandPort;
        bool _isGuidingCameraMotion;
        uint _cameraLivefeedCallbackKey;
        OfficerSearchState _searchState;
        OfficerDirection _lastSeen;
        void SendMoveCommand(uchar specifierByte, double horizontal, double vertical, string moveName);
        void CheckLastSeen();
        void OnLivefeedImageReceived(LiveFeedCallbackArgs args);
        static int AngleToMotorValue(double angle);
    };

    enum CommandAction
    {
        Ping = 1,
        StartOfficerTracking = 2,
        StopOfficerTracking = 3,
        SendKeyword = 4,
        RelativeMoveSynchronous = 5,
        RelativeMoveAsynchronous = 6,
        AbsoluteMoveSynchronous = 7,
        AbsoluteMoveAsynchronous = 8,
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
        vector<uchar> ReadResponse(Device device);
        bool TryReadResponse(Device device, vector<uchar>* readResponse);
        void SendResponse(vector<uchar> formattedResponse);
        void SendResponse(uchar formattedResponse);
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
        Vector2 TargetRegionProportion;
        Vector2 SafeRegionProportion;
        int CameraFramesToSkipMoving;
        double CameraFrameRate;
        int CameraFrameWidth;
        int CameraFrameHeight;
        void Load(string settingsFile);

    private:
        static Vector2 ReadVector2(Document& doc, string vectorName);
    };


    enum LogFlag
    {
        Error = 0b1,
        Debug = 0b11,
        Information = 0b111,
        Frames = 0b10001,
        Officers = 0b100001,
        Movements = 0b1000001,
        Recording = 145,
        RawSerial = 256,
        DeviceSerial = 512,
        Acknowledge = 1792
    };

    void Log(string s, uint flags);
    void ConfigureLog(uint flags);
}