#include "imaging.hpp"

using namespace tsw::imaging;
using namespace cv;

SmartOfficerLocator::SmartOfficerLocator(int16_t officerClassId) : OfficerLocator(officerClassId) 
{
    MaxHSV[0] = 179;
    MaxHSV[1] = 255;
    MaxHSV[2] = 255;
    OfficerThreshold = 0.15;
}

Vector2* SmartOfficerLocator::GetDesiredOfficerLocation(ImagePtr image)
{
    vector<OfficerInferenceBox> boxes = GetOfficerLocations(image);
    Log("Found " + to_string(boxes.size()) + " bounding boxes", Officers);

    unsigned char* data = (unsigned char*)image->GetData();
    Mat m(image->GetHeight(), image->GetWidth(), CV_8UC3, data);

    OfficerInferenceBox* bestBox = nullptr;
    for(int i = 0; i < boxes.size(); i++)
    {
        OfficerInferenceBox curBox = boxes[i];
        
        // Determine how big the roi is.
        Log("ROI: Top-Left = (" + to_string(curBox.topLeftX) + ", " + to_string(curBox.topLeftY) + ") Bottom-Right = (" + to_string(curBox.bottomRightX) + ", " + to_string(curBox.bottomRightY) + ")", OpenCV);
        Size roiSize(curBox.bottomRightX - curBox.topLeftX, curBox.bottomRightY - curBox.topLeftY);
        
        Log("Creating ROI matrix", OpenCV);
        Mat roi(roiSize, CV_8UC3);
        Log("ROI matrix created", OpenCV);

        // This places just the bounding box in a seperate mat.
        m(Rect(Point(curBox.topLeftX, curBox.topLeftY), roiSize)).copyTo(roi(Rect(Point(0, 0), roiSize)));

        // Now we gotta convert this guy to HSV.
        cvtColor(roi, roi, COLOR_RGB2HSV);

        // Now we can see how much of the image is in range of out threshold.
        Mat hsvThreshold;
        inRange(roi, MinHSV, MaxHSV, hsvThreshold);

        // At least 30% of the box has to be in the threshold for us to consider it.
        int numInThreshold = 0;
        int total = 0;
        for(int row = 0; row < hsvThreshold.rows; row += 10)
        {
            uchar* rowPtr = hsvThreshold.ptr(row);
            for(int col = 0; col < hsvThreshold.cols; col += 10)
            {
                if(rowPtr[col] > 0)
                {
                    numInThreshold++;
                }

                total++;
            }
        }

        float thresholdProp = total != 0 ? numInThreshold / (float)(total) : 0;
        Log("Officer threshold value of " + to_string(thresholdProp), Officers);
        if(thresholdProp >= OfficerThreshold)
        {
            // This box has enough of the color. Now let's compare it.
            if(!bestBox || boxes[i].confidence > bestBox->confidence)
            {
                bestBox = &boxes[i];
            }
        }
    }

    // If our best box is still null, we didn't find an officer.
    if(!bestBox)
    {
        return NULL;
    }

    Log("Highest Confidence: " + to_string(bestBox->confidence), Officers);

    Vector2* p = new Vector2();
    p->x = (bestBox->topLeftX + bestBox->bottomRightX) / 2.0;
    p->y = (bestBox->topLeftY + bestBox->bottomRightY) / 2.0;
    return p;
}
