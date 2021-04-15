#include "imaging.hpp"
#include "settings.hpp"

#define MAX_H 180
#define MAX_S 255
#define MAX_V 255
#define WINDOW "Filter Calibrate"
#define H_MIN_TRACKBAR "H Min"
#define H_MAX_TRACKBAR "H Max"
#define S_MIN_TRACKBAR "S Min"
#define S_MAX_TRACKBAR "S Max"
#define V_MIN_TRACKBAR "V Min"
#define V_MAX_TRACKBAR "V Max"

using namespace tsw::imaging;
using namespace tsw::io::settings;
using namespace cv;

bool isFiltering = false;

int lowH = 0, highH = MAX_H;
int lowS = 0, highS = MAX_S;
int lowV = 0, highV = MAX_V;

void OnTrackbarLowH(int, void*)
{
    lowH = min(highH - 1, lowH);
    setTrackbarPos(H_MIN_TRACKBAR, WINDOW, lowH);
}

void OnTrackbarHighH(int, void*)
{
    highH = max(highH, lowH + 1);
    setTrackbarPos(H_MAX_TRACKBAR, WINDOW, highH);
}

void OnTrackbarLowS(int, void*)
{
    lowS = min(highS - 1, lowS);
    setTrackbarPos(S_MIN_TRACKBAR, WINDOW, lowS);
}

void OnTrackbarHighS(int, void*)
{
    highS = max(highS, lowS + 1);
    setTrackbarPos(S_MAX_TRACKBAR, WINDOW, highS);
}

void OnTrackbarLowV(int, void*)
{
    lowV = min(highV - 1, lowV);
    setTrackbarPos(V_MIN_TRACKBAR, WINDOW, lowV);
}

void OnTrackbarHighV(int, void*)
{
    highV = max(highV, lowV + 1);
    setTrackbarPos(V_MAX_TRACKBAR, WINDOW, highV);
}

void OnLiveFeedImageReceived(LiveFeedCallbackArgs args)
{
    // Put the pointer into an array of bytes.
    unsigned char* data = (unsigned char*)args.image->GetData();
    Mat m(args.image->GetHeight(), args.image->GetWidth(), CV_8UC3, data);

    // Opencv interperets the data as bgr instead of rgb, so we gotta convert it.
    cvtColor(m, m, COLOR_RGB2HSV);
    inRange(m, Scalar(lowH, lowS, lowV), Scalar(highH, highS, highV), m);

    imshow(WINDOW, m);
    char key = (char)waitKey(20);
    if(key == 'q' || key == 27)
    {
        isFiltering = false;
    }
}

int main(int argc, char* argv[])
{
    string thisFile(argv[0]);
    string startingDir = thisFile.substr(0, thisFile.find_last_of('/'));
    string settingsFile = startingDir + "/tsw.json";
    TswSettings settings(settingsFile);

    lowH = (int)settings.MinOfficerHSV[0];
    lowS = (int)settings.MinOfficerHSV[1];
    lowV = (int)settings.MinOfficerHSV[2];
    highH = (int)settings.MaxOfficerHSV[0];
    highS = (int)settings.MaxOfficerHSV[1];
    highV = (int)settings.MaxOfficerHSV[2];


    FlirCamera camera(settings.CameraBufferCount);

    cout << "Connecting to camera..." << endl;
    camera.Connect("20386745");
    camera.SetFrameHeight(settings.CameraFrameHeight);
    camera.SetFrameWidth(settings.CameraFrameWidth);
    camera.SetFrameRate(settings.CameraFrameRate);
    cout << "Camera connected" << endl;

    namedWindow(WINDOW);
    createTrackbar(H_MIN_TRACKBAR, WINDOW, &lowH, MAX_H, OnTrackbarLowH);
    createTrackbar(H_MAX_TRACKBAR, WINDOW, &highH, MAX_H, OnTrackbarHighH);
    createTrackbar(S_MIN_TRACKBAR, WINDOW, &lowS, MAX_S, OnTrackbarLowS);
    createTrackbar(S_MAX_TRACKBAR, WINDOW, &highS, MAX_S, OnTrackbarHighS);
    createTrackbar(V_MIN_TRACKBAR, WINDOW, &lowV, MAX_V, OnTrackbarLowV);
    createTrackbar(V_MAX_TRACKBAR, WINDOW, &highV, MAX_V, OnTrackbarHighV);

    uint key = camera.RegisterLiveFeedCallback(&OnLiveFeedImageReceived);
    isFiltering = true;
    camera.StartLiveFeed();

    while(isFiltering)
    {
        usleep(100000);
    }

    camera.UnregisterLiveFeedCallback(key);
    camera.StopLiveFeed();

    return 0;
}