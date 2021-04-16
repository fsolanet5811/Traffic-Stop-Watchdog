#pragma once

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include "opencv2/opencv.hpp"
#include "utilities.hpp"
#include "io.hpp"
#include "common.hpp"
#include <string>
#include <future>

using namespace std;
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace cv;
using namespace tsw::utilities;
using namespace tsw::io;
using namespace tsw::common;

namespace tsw::imaging
{
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
        FlirCamera(int bufferCount);
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
        int _bufferCount;
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
        Recorder(Size frameSize, double fps);
        void StartRecording(string fileName);
        void StopRecording();
        bool IsRecording();
        void AddFrame(Mat frame);

    private:
        bool _isRecording;
        string _recordedFileName;
        uint _callbackKey;
        VideoWriter _aviWriter;
        queue<Mat> _frameBuffer;
        SmartLock _frameBufferLock;
        future<void> _recordFuture;
        Size _frameSize;
        double _fps;
        void Record();
    };

    class OfficerLocator
    {
    public:
        int16_t OfficerClassId;
        Vector2 TargetRegionProportion;
        Vector2 SafeRegionProportion;
        float ConfidenceThreshold;
        OfficerDirection FindOfficer(ImagePtr image);
        OfficerDirection FindOfficer(ImagePtr image, OfficerInferenceBox* officerBox);
        OfficerInferenceBox* GetOfficerBox(ImagePtr image);
        vector<OfficerInferenceBox> GetOfficerLocations(ImagePtr image);

    protected:
        OfficerLocator(int16_t officerClassId);
        virtual OfficerInferenceBox* GetDesiredOfficerBox(vector<OfficerInferenceBox> officerBoxes, ImagePtr image) = 0;    

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
        static short CleanCoordinate(short coordinate, short max);
    };

    class ConfidenceOfficerLocator : public OfficerLocator
    {
    public:
        ConfidenceOfficerLocator(int16_t OfficerClassId);

    protected:
        OfficerInferenceBox* GetDesiredOfficerBox(vector<OfficerInferenceBox> officerBoxes, ImagePtr image);
    };

    class SmartOfficerLocator : public OfficerLocator
    {
    public:
        SmartOfficerLocator(int16_t OfficerClassId);
        Scalar MinHSV;
        Scalar MaxHSV;
        double OfficerThreshold;
    
    protected:
        OfficerInferenceBox* GetDesiredOfficerBox(vector<OfficerInferenceBox> officerBoxes, ImagePtr image);
    };

    class TestOfficerLocator : public OfficerLocator
    {
    public:
        TestOfficerLocator(int16_t OfficerClassId);

    protected:
        OfficerInferenceBox* GetDesiredOfficerBox(vector<OfficerInferenceBox> officerBoxes, ImagePtr image);

    private:
        int _status;
    };

    class DisplayWindow
    {
    public:
        DisplayWindow(string windowName, int refreshRate);
        string WindowName;
        void Show();
        void Update(Mat currentFrame);
        void Close();
        bool IsShown();

    private:
        bool _isShown;
        future<void> _showFuture;
        Mat _currentFrame;
        int _refreshRate;
        SmartLock _displayLock;
        Mat MatFromImage(ImagePtr image);
        void RunWindow();
    };

    class ImageProcessor
    {
    public:
        ImageProcessor(DisplayWindow& window, FlirCamera& camera, SmartOfficerLocator& officerLocator, CameraMotionController& motionController, ImageProcessingConfig config);
        uint CameraFramesToSkip;
        void StartProcessing();
        void StopProcessing();
        bool IsProcessing();
        ~ImageProcessor();

    private:
        Recorder* _footageRecorder;
        Recorder* _filterRecorder;
        DisplayWindow* _window;
        FlirCamera* _camera;
        SmartOfficerLocator* _officerLocator;
        CameraMotionController* _motionController;
        uint _livefeedCallbackKey;
        bool _isProcessing;
        ImageProcessingConfig _config;
        void OnLiveFeedImageReceived(LiveFeedCallbackArgs args);
        void DrawOfficerBox(OfficerInferenceBox* box, Mat cvImage, Scalar color);
        Mat MatFromImage(ImagePtr image, OfficerInferenceBox* officerBox);
    };
}