#include "imaging.hpp"

using namespace tsw::imaging;
using namespace Spinnaker;

TestOfficerLocator::TestOfficerLocator(int16_t officerClassId) : OfficerLocator(officerClassId) { }

OfficerInferenceBox* TestOfficerLocator::GetDesiredOfficerBox(vector<OfficerInferenceBox> officerBoxes, ImagePtr image)
{
    Vector2* location = new Vector2();
    int mode = _status / 20;
    if(mode == 0)
    {
        // These will be the top left.
        location->x = 0;
        location->y = 0;
    }
    else if(mode == 1)
    {
        // These will be top right.
        location->x = image->GetWidth();
        location->y = 0;
    }
    else if(mode == 2)
    {
        // These will be bottom right.
        location->x = image->GetWidth();
        location->y = image->GetHeight();
    }
    else
    {
        // These will be bottom left.
        location->x = 0;
        location->y = image->GetHeight();
    }

    // Change the status so we can see new coordinates.
    if(_status == 80)
    {
        _status = 0;
    }
    else
    {
        _status++;
    }

    delete location;
    return nullptr;
}