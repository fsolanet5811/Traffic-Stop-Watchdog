#pragma once

#include "Spinnaker.h"
#include "rapidjson.h"
#include "common.hpp"
#include <string>
#include "document.h"
#include "utilities.hpp"
#include <termios.h>
#include <future>

#define LED_ON 255
#define LED_OFF 0
#define FLASH_ON_TIME 200000
#define FLASH_OFF_TIME 200000

using namespace Spinnaker;
using namespace std;
using namespace rapidjson;
using namespace tsw::common;
using namespace tsw::utilities;

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
        static string ToHex(unsigned char* bytes, int numBytes);
        
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
        vector<unsigned char> bytes;
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
        Activate = 9,
        Deactivate = 10,
        SetSpeeds = 11,
        Acknowledge = 15
    };


    class DeviceSerialPort
    {
    public:
        DeviceSerialPort(SerialPort& port);
        void StartGathering();
        void StopGathering();
        bool IsGathering();
        DeviceMessage ReadFromDevice(Device device);
        DeviceMessage ReadFromDevice(Device device, unsigned char header);
        void WriteToDevice(Device device, CommandAction command, vector<unsigned char> data);
        void WriteToDevice(Device device, CommandAction command);
        bool TryReadFromDevice(Device device, DeviceMessage* readMessage);
        ~DeviceSerialPort();

    private:
        bool _isGathering;
        future<void> _gatherFuture;
        SerialPort* _port;
        vector<DeviceMessage> _buffer;
        SmartLock _bufferLock;
        void Gather();
        void WriteToDevice(vector<unsigned char> formattedData);
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
        void SetSpeeds(ByteVector2 speeds);
        bool TryReadMessage(DeviceMessage* message);

    private:
        DeviceSerialPort* _commandPort;
        void SendMoveCommand(CommandAction moveType, double horizontal, double vertical, string moveName);
        void ReadAcknowledge();
        void ReadSuccess();
        static int AngleToMotorValue(double angle, MotorConfig config);
    };

    class CameraMotionController
    {
    public:
        CameraMotionController(MotorController& motorController);
        double VerticalFov;
        double HorizontalFov;
        Vector2 HomeAngles;
        Bounds AngleXBounds;
        ByteVector2 MotorSpeeds;
        void InitializeGuidance();
        void UninitializeGuidance();
        bool IsGuidanceInitialized();
        void GuideCameraTo(OfficerDirection location);
        void OfficerSearch();
        void GoToHome();
        static int GetMaxValue();

    private:
        enum OfficerSearchState
        {
            NotSearching = 0,
            CheckingLastSeen = 1,
            Circling = 2
        };
        MotorController* _motorController;
        bool _isGuidanceInitialized;
        uint _cameraLivefeedCallbackKey;
        OfficerSearchState _searchState;
        OfficerDirection _lastSeen;
        bool _movingTowardsMin;
        void ResetSearchState();
        void SendMoveCommand(unsigned char specifierByte, double horizontal, double vertical, string moveName);
        void CheckLastSeen();
        void MoveToMin();
        void MoveToMax();
        void Circle();
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
        bool TryReadResponse(Device device, vector<unsigned char>* readResponse);
        void SendResponse(vector<unsigned char> formattedResponse);
        void SendCommand(Device device, Command* command);
        void AcknowledgeReceived(Device device);
        vector<unsigned char> ReadResponse(Device device);
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
        void RunFlash();
        void SetBrightness(unsigned char brightness);
    };
}
