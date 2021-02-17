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

    class CameraMotionController
    {
    public:
        CameraMotionController(FlirCamera& camera, OfficerLocator& officerLocator, DeviceSerialPort& commandPort);
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
        
        void ResetSearchState();
        void SendMoveCommand(uchar specifierByte, double horizontal, double vertical, string moveName);
        void CheckLastSeen();
        void OnLivefeedImageReceived(LiveFeedCallbackArgs args);
        static int AngleToMotorValue(double angle, MotorConfig config);
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
        virtual Command* ReadCommand(Device device);
        virtual bool TryReadResponse(Device device, vector<uchar>* readResponse);
        virtual void SendResponse(vector<uchar> formattedResponse);
        virtual void SendCommand(Device device, Command* command);
        virtual void AcknowledgeReceived(Device device);
        vector<uchar> ReadResponse(Device device);
        void SendResponse(uchar formattedResponse);
        virtual ~CommandAgent();

    protected:
        DeviceSerialPort* _commandPort;
    };

    

    class MultiPortCommandAgent : public CommandAgent
    {
    public:
        MultiPortCommandAgent(DeviceSerialPort& handheldPort, DeviceSerialPort& motorsPort);
        Command* ReadCommand(Device device) override;
        bool TryReadResponse(Device device, vector<uchar>* readResponse) override;
        void SendResponse(vector<uchar> formattedResponse) override;
        void SendCommand(Device device, Command* command) override;
        void AcknowledgeReceived(Device device) override;
        ~MultiPortCommandAgent() override;

    private:
        DeviceSerialPort* _handheldPort;
        DeviceSerialPort* _motorsPort;
        DeviceSerialPort* GetDeviceSerialPort(Device device);
    };

    class Settings
    {
    public:
        Settings();
        Settings(string settingsFile);
        string DeviceSerialPath;
        string MotorsSerialPath;
        string HandheldSerialPath;
        string CameraSerialNumber;
        bool UseDeviceAdapter;
        short OfficerClassId;
        Vector2 TargetRegionProportion;
        Vector2 SafeRegionProportion;
        int CameraFramesToSkipMoving;
        double CameraFrameRate;
        int CameraFrameWidth;
        int CameraFrameHeight;
        uint LogFlags;
        MotorConfig PanConfig;
        MotorConfig TiltConfig;
        void Load(string settingsFile);

    private:
        static Vector2 ReadVector2(Document& doc, string vectorName);
        static bool ReadLogFlag(Document& doc, string flagName);
        static Bounds ReadBounds(Document& doc, string boundsName);
        static MotorConfig ReadMotorConfig(Document& doc, string motorConfigName);
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