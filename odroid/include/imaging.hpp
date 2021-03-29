#pragma once

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include "opencv2/opencv.hpp"
#include "utilities.hpp"
#include <string>
#include <future>

using namespace std;
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace cv;
using namespace tsw::utilities;

namespace tsw::imaging
{
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

    struct LiveFeedCallbackArgs
    {
        ImagePtr image;
        size_t imageIndex;
    }; 

    struct LiveFeedCallback
    {
        function<void(LiveFeedCallbackArgs)> callback;
        uint callbackKey;
    };

    class FlirCamera
    {
    public:
        FlirCamera();
        ~FlirCamera();
        vector<string> FindDevices();
        void Connect(string serialNumber);
        double GetDeviceTemperature();
        double GetFrameRate();
        int GetFrameHeight();
        int GetFrameWidth();
        void SetFrameHeight(int frameHeight);
        void SetFrameWidth(int frameWidth);
        void SetFrameRate(double hertz);
        void StartLiveFeed();
        void StopLiveFeed();
        bool IsLiveFeedOn();
        uint RegisterLiveFeedCallback(function<void(LiveFeedCallbackArgs)> callback);
        void UnregisterLiveFeedCallback(uint callbackKey);
        ImagePtr CaptureImage();
        void SetFilter(RgbTransformLightSourceEnums filter);

    private:
        SystemPtr _system;
        CameraPtr _camera;
        SmartLock _liveFeedLock;
        uint _nextLiveFeedKey;
        bool _isLiveFeedOn;
        list<LiveFeedCallback> _liveFeedCallbacks;
        future<void> _liveFeedFuture;
        bool _isConnected;
        bool _shouldBeConnected;
        string _connectedSerialNumber;
        int* _userFrameHeight;
        int* _userFrameWidth;
        double* _userFrameRate;
        RgbTransformLightSourceEnums* _userFilter;

        void RunLiveFeed();
        void OnLiveFeedImageReceived(ImagePtr image, uint imageIndex);
        bool TryConnect(string serialNumber, CameraPtr* camera);
        void WaitForConnected(bool attemptToConnect = false);
        void EnsureConnectionNotLost();
        void PowerCycle();
    };

    class Recorder
    {
    public:
        Recorder(FlirCamera& camera);
        void StartRecording(string fileName);
        void StopRecording();
        bool IsRecording();

    private:
        FlirCamera* _camera;
        future<void> _recordingFuture;
        bool _isRecording;
        string _recordedFileName;
        uint _callbackKey;
        VideoWriter _aviWriter;
        queue<ImagePtr> _frameBuffer;
        SmartLock _frameBufferLock;
        future<void> _recordFuture;

        void Record();
        void OnLiveFeedImageReceived(LiveFeedCallbackArgs args);
        Mat MatFromImage(ImagePtr image);
    };

    class OfficerLocator
    {
    public:
        int16_t OfficerClassId;
        Vector2 TargetRegionProportion;
        Vector2 SafeRegionProportion;
        OfficerDirection FindOfficer(ImagePtr image);

    protected:
        OfficerLocator(int16_t officerClassId);
        vector<InferenceBoundingBox> GetOfficerLocations(ImagePtr image);
        virtual Vector2* GetDesiredOfficerLocation(ImagePtr image) = 0;    

    private:
        enum RegionLocation
        {
            None,
            Target,
            Safe
        };
        bool _isTravelingToTarget;
        RegionLocation _lastLocation;
        RegionLocation GetRegionLocation(Vector2 location, ImagePtr image);
        static bool IsPointInRegion(Vector2 location, Vector2 region, ImagePtr image);
    };

    class ConfidenceOfficerLocator : public OfficerLocator
    {
    public:
        ConfidenceOfficerLocator(int16_t OfficerClassId);

    protected:
        Vector2* GetDesiredOfficerLocation(ImagePtr image);
    };

    class TestOfficerLocator : public OfficerLocator
    {
    public:
        TestOfficerLocator(int16_t OfficerClassId);

    protected:
        Vector2* GetDesiredOfficerLocation(ImagePtr image);

    private:
        int _status;
    };

    class DisplayWindow
    {
    public:
        DisplayWindow(FlirCamera& camera, string windowName, int refreshRate);
        string WindowName;
        void Show();
        void Close();
        bool IsShown();

    private:
        FlirCamera* _camera;
        uint _liveFeedCallbackKey;
        bool _isShown;
        future<void> _showFuture;
        Mat _currentFrame;
        int _refreshRate;
        SmartLock _displayLock;
        Mat MatFromImage(ImagePtr image);
        void OnLiveFeedImageReceived(LiveFeedCallbackArgs args);
        void RunWindow();
    };
}