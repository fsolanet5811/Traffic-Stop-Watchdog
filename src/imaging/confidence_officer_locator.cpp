#include "imaging.hpp"

using namespace tsw::imaging;
using namespace Spinnaker;

ConfidenceOfficerLocator::ConfidenceOfficerLocator(int16_t officerClassId) : OfficerLocator(officerClassId) { }

Point* ConfidenceOfficerLocator::GetDesiredOfficerLocation(ImagePtr image)
{
    // Grab all the bounding boxes.
    vector<InferenceBoundingBox> boxes = GetOfficerLocations(image);

    // We are going to take the one with the most confidence.
    InferenceBoundingBox* bestBox = NULL;
    for(int i = 0; i < boxes.size(); i++)
    {
        if(!bestBox || boxes[i].confidence > bestBox->confidence)
        {
            bestBox = &boxes[i];
        }
    }

    // If our best box is still null, we didn't find an officer.
    if(!bestBox)
    {
        return NULL;
    }

    Point* p = new Point();
    p->x = (bestBox->rect.topLeftXCoord + bestBox->rect.bottomRightXCoord) / 2.0;
    p->y = (bestBox->rect.topLeftYCoord + bestBox->rect.bottomRightYCoord) / 2.0;
    return p;
}