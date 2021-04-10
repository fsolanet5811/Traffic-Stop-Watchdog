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

OfficerInferenceBox* SmartOfficerLocator::GetDesiredOfficerBox(vector<OfficerInferenceBox> officerBoxes, ImagePtr image)
{
    unsigned char* data = (unsigned char*)image->GetData();
    Mat m(image->GetHeight(), image->GetWidth(), CV_8UC3, data);

    OfficerInferenceBox* bestBox = nullptr;
    for(int i = 0; i < officerBoxes.size(); i++)
    {
        OfficerInferenceBox curBox = officerBoxes[i];
        
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
            if(!bestBox || officerBoxes[i].confidence > bestBox->confidence)
            {
                delete bestBox;
                bestBox = new OfficerInferenceBox(officerBoxes[i]);
            }
        }
    }

    // If our best box is still null, we didn't find an officer.
    if(bestBox)
    {
        Log("Officer Confidence: " + to_string(bestBox->confidence), Officers);
    }

    return bestBox;
}
