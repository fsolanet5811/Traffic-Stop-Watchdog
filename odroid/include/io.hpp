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
        void Clear();
        void Close();
        ~SerialPort();
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
        ~DeviceSerialPort();

    private:
        bool _isGathering;
        future<void> _gatherFuture;
        SerialPort* _port;
        vector<DeviceMessage> _buffer;
        mutex _bufferKey;
        void Gather();
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

    class MotorController
    {
    public:
        MotorController(DeviceSerialPort& commandPort, MotorConfig panConfig, MotorConfig tiltConfig);
        MotorConfig PanConfig;
        MotorConfig TiltConfig;
        void SendSyncRelativeMoveCommand(double horizontal, double vertical);
        void SendAsyncRelativeMoveCommand(double horizontal, double vertical);
        void SendSyncAbsoluteMoveCommand(double horizontal, double vertical);
        void SendAsyncAbsoluteMoveCommand(double horizontal, double vertical);
        void Activate();
        void Deactivate();
        bool TryReadMessage(DeviceMessage* message);

    private:
        DeviceSerialPort* _commandPort;
        void SendMoveCommand(uchar specifierByte, double horizontal, double vertical, string moveName);
        static int AngleToMotorValue(double angle, MotorConfig config);
    };

    class CameraMotionController
    {
    public:
        CameraMotionController(FlirCamera& camera, OfficerLocator& officerLocator, MotorController& motorController);
        double VerticalFov;
        double HorizontalFov;
        Vector2 HomeAngles;
        uint CameraFramesToSkip;
        MotorConfig PanConfig;
        MotorConfig TiltConfig;
        void StartCameraMotionGuidance();
        void StopCameraMotionGuidance();
        bool IsGuidingCameraMotion();
        void OfficerSearch();
        void GoToHome();
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
        MotorController* _motorController;
        bool _isGuidingCameraMotion;
        uint _cameraLivefeedCallbackKey;
        OfficerSearchState _searchState;
        OfficerDirection _lastSeen;
        
        void ResetSearchState();
        void SendMoveCommand(uchar specifierByte, double horizontal, double vertical, string moveName);
        void CheckLastSeen();
        void OnLivefeedImageReceived(LiveFeedCallbackArgs args);
        
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
        bool TryReadResponse(Device device, vector<uchar>* readResponse);
        void SendResponse(vector<uchar> formattedResponse);
        void SendCommand(Device device, Command* command);
        void AcknowledgeReceived(Device device);
        vector<uchar> ReadResponse(Device device);
        void SendResponse(uchar formattedResponse);
        ~CommandAgent();

    protected:
        DeviceSerialPort* _commandPort;
    };

    
    enum LogFlag
    {
        Error = 0b1,
        Debug = 0b10,
        Information = 0b100,
        Frames = 0b1000,
        Officers = 0b10000,
        Movements = 0b100000,
        Recording = 0b1000000,
        RawSerial = 0b10000000,
        DeviceSerial = 0b100000000,
        Acknowledge = 0b1000000000
    };
}

void ConfigureLog(uint flags);
void Log(string s, uint flags);