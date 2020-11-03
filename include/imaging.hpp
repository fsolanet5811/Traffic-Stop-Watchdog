#pragma once

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include "SpinVideo.h"
#include <string>
#include <future>

using namespace std;
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::Video;

namespace tsw::imaging
{
    struct Point
    {
        double x;
        double y;
    };

    struct OfficerDirection
    {
        bool foundOfficer;
        Point movement;
    };

    struct LiveFeedCallbackArgs
    {
        ImagePtr image;
        uint imageIndex;
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
        vector<string> FindDevices();
        void Connect(string serialNumber);
        double GetDeviceTemperature();
        double GetFrameRate();
        void SetFrameRate(double hertz);
        void StartLiveFeed();
        void StopLiveFeed();
        bool IsLiveFeedOn();
        uint RegisterLiveFeedCallback(function<void(LiveFeedCallbackArgs)> callback);
        void UnregisterLiveFeedCallback(uint callbackKey);

    private:
        SystemPtr _system;
        CameraPtr _camera;
        mutex _liveFeedKey;
        future<void> _liveFeedFuture;
        uint _nextLiveFeedKey;
        bool _isLiveFeedOn;
        list<LiveFeedCallback> _liveFeedCallbacks;

        void RunLiveFeed();
        void OnLiveFeedImageReceived(ImagePtr image, uint imageIndex); 
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
        SpinVideo _recordedVideo;

        void Record();
        void OnLiveFeedImageReceived(LiveFeedCallbackArgs args);
    };

    class OfficerLocator
    {
    public:
        int16_t OfficerClassId;
        Point TargetRegionProportion;
        Point SafeRegionProportion;
        OfficerDirection FindOfficer(ImagePtr image);

    protected:
        OfficerLocator(int16_t officerClassId);
        vector<InferenceBoundingBox> GetOfficerLocations(ImagePtr image);
        virtual Point* GetDesiredOfficerLocation(ImagePtr image) = 0;    

    private:
        enum RegionLocation
        {
            None,
            Target,
            Safe
        };
        bool _isTravelingToTarget;
        RegionLocation _lastLocation;
        RegionLocation GetRegionLocation(Point location, ImagePtr image);
        static bool IsPointInRegion(Point location, Point region, ImagePtr image);
    };

    class ConfidenceOfficerLocator : public OfficerLocator
    {
    public:
        ConfidenceOfficerLocator(int16_t OfficerClassId);

    protected:
        Point* GetDesiredOfficerLocation(ImagePtr image);
    };
}