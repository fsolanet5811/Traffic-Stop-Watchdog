#pragma once

#include "Spinnaker.h"
#include "imaging.hpp"
#include "rapidjson.h"
#include "document.h"
#include <termios.h>
#include <future>

#define LED_ON 255
#define LED_OFF 0
#define FLASH_ON_TIME 200000
#define FLASH_OFF_TIME 200000
#define FLASH_PAUSE_TIME 750000

using namespace Spinnaker;
using namespace tsw::imaging;
using namespace std;
using namespace rapidjson;

namespace tsw::io
{
    class SerialPort
    {
    public:
        SerialPort(speed_t baudRate);
        void Open(string devicePath);
        int Read(unsigned char* buffer, int bytesToRead);
        int Write(unsigned char* data, int bytesToWrite);
        void Clear();
        void Close();
        ~SerialPort();
    private:
	    speed_t _baudRate;
        int _port;
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
        SmartLock _bufferLock;
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

    struct SerialConfig
    {
        string path;
        speed_t baudRate;
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

    class StatusLED
    {
    public:
        StatusLED(string ledFile);
        int FlashesPerPause;
        useconds_t PauseTime;
        void StartFlashing(int flashesPerPause);
        void StartFlashing();
        void StopFlashing(bool reset = true);
        bool IsFlashing();
        bool IsEnabled();
        void SetEnabled(bool enabled);

    private:
        string _ledFile;
        bool _isFlashing;
        bool _isEnabled;
        future<void> _flashFuture;
        void RunFlash(int flashesPerPause);
        void SetBrightness(uchar brightness);
    };
}
