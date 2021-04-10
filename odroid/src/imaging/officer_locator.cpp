#include "imaging.hpp"
#include "Spinnaker.h"
#include "io.hpp"

using namespace tsw::imaging;
using namespace tsw::io;
using namespace Spinnaker;

OfficerLocator::OfficerLocator(int16_t officerClassId)
{
    OfficerClassId = officerClassId;

    // We accept all boxes by default.
    ConfidenceThreshold = 0;
}

OfficerDirection OfficerLocator::FindOfficer(ImagePtr image)
{
    // Determine where on the image the officer is.
    OfficerInferenceBox* officerBox = GetOfficerBox(image);

    if(!officerBox)
    {
        // No officers were found on the image.
        Log("No officers found", Officers);
        
        OfficerDirection res;
        res.foundOfficer = false;
        return res;
    }

    OfficerDirection dir = FindOfficer(image, officerBox);

    // Remove the unmanaged box.
    delete officerBox;

    return dir;
}

OfficerDirection OfficerLocator::FindOfficer(ImagePtr image, OfficerInferenceBox* officerBox)
{
    // This will hold the result.
    OfficerDirection res;

    // They may have given us a null box.
    if(!officerBox)
    {
        res.foundOfficer = false;
        return res;
    }

    // Get a vecot that points from the center of the frame to the center of the box.
    Vector2 officerLoc;
    officerLoc.x = (officerBox->topLeftX + officerBox->bottomRightX) / 2.0;
    officerLoc.y = (officerBox->topLeftY + officerBox->bottomRightY) / 2.0;

    // We have a location, now determine if we actually have to get there.
    // This is taking into account the region we found the officer in and the last region the officer was in.
    RegionLocation region = GetRegionLocation(officerLoc, image);

    // The two cases that warrant no moving are:
    // 1. We are in the target region
    // 2. We are in the safe region and are not currently attempting to move to the target region.
    if(region == Target || (region == Safe && !_isTravelingToTarget))
    {
        // We have to reset the flag that indicates we are attempting to move to the target, otherwise the officer will move to target once it hits the safe region.
        _isTravelingToTarget = false;
        res.shouldMove = false;
    }
    else
    {
        // Now we actually have to do some work.
        // The goal is to get the officer to the center of the image.
        _isTravelingToTarget = true;
        res.shouldMove = true;
    }
    
    // First transform the location of the officer into [-1, 1] space (from left to right).
    // Keep in mind that we have to reverse the y axis.
    officerLoc.x = officerLoc.x / (image->GetWidth() / 2) - 1;
    officerLoc.y = 1 - officerLoc.y / (image->GetHeight() / 2);

    // That actually is the direction we want to move the officer.
    // Just transfor the point data over and we can delete the unmanaged point.
    res.movement.x = officerLoc.x;
    res.movement.y = officerLoc.y;

    return res;
}

OfficerInferenceBox* OfficerLocator::GetOfficerBox(ImagePtr image)
{
    vector<OfficerInferenceBox> boxes = GetOfficerLocations(image);
    Log("Found " + to_string(boxes.size()) + " bounding boxes", Officers);
    return GetDesiredOfficerBox(boxes, image);
}

vector<OfficerInferenceBox> OfficerLocator::GetOfficerLocations(ImagePtr image)
{
    // This will hold all of the bounding boxes.
    vector<OfficerInferenceBox> boxes;

    // Iterate over all of the boxes and add them to our vector;
    InferenceBoundingBoxResult boxRes = image->GetChunkData().GetInferenceBoundingBoxResult();
    for(int i = 0; i < boxRes.GetBoxCount(); i++)
    {
        // We only want the boxes that coorespond to the officer class and have a certain amount of confidence.
        InferenceBoundingBox box = boxRes.GetBoxAt(i);
        if(box.classId == OfficerClassId && box.confidence >= ConfidenceThreshold)
        {
            OfficerInferenceBox officerBox;
            officerBox.confidence = box.confidence;

            // The coordinates need to be cleaned up because flir thought it would be a cool idea to allow them to exist outside the frame!
            officerBox.bottomRightX = CleanCoordinate(box.rect.bottomRightXCoord, image->GetWidth() - 1);
            officerBox.bottomRightY = CleanCoordinate(box.rect.bottomRightYCoord, image->GetHeight() - 1);
            officerBox.topLeftX = CleanCoordinate(box.rect.topLeftXCoord, image->GetWidth() - 1);
            officerBox.topLeftY = CleanCoordinate(box.rect.topLeftYCoord, image->GetHeight() - 1);

            boxes.push_back(officerBox);
        }
    }

    return boxes;
}

OfficerLocator::RegionLocation OfficerLocator::GetRegionLocation(Vector2 location, ImagePtr image)
{
    if(IsPointInRegion(location, TargetRegionProportion, image))
    {
        return Target;
    }

    if(IsPointInRegion(location, SafeRegionProportion, image))
    {
        return Safe;
    }

    // We are not in any regions.
    return None;
}

bool OfficerLocator::IsPointInRegion(Vector2 location, Vector2 region, ImagePtr image)
{
    double regionLeft = (0.5 - region.x / 2) * image->GetWidth();
    if(location.x > regionLeft)
    {
        double regionRight = (0.5 + region.x / 2) * image->GetWidth();
        if(location.x < regionRight)
        {
            double regionTop = (0.5 - region.x / 2) * image->GetHeight();
            if(location.y > regionTop)
            {
                double regionBottom = (0.5 + region.y / 2) * image->GetHeight();
                return location.y < regionBottom;
            }
        }
    }

    return false;
}

short OfficerLocator::CleanCoordinate(short coordinate, short max)
{
    return std::max(std::min(max, coordinate), (short)0);
}
