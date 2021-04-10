#include "imaging.hpp"
#include "io.hpp"

using namespace tsw::imaging;
using namespace tsw::io;
using namespace Spinnaker;

ConfidenceOfficerLocator::ConfidenceOfficerLocator(int16_t officerClassId) : OfficerLocator(officerClassId) { }

OfficerInferenceBox* ConfidenceOfficerLocator::GetDesiredOfficerBox(vector<OfficerInferenceBox> officerBoxes, ImagePtr image)
{
    // We are going to take the one with the most confidence.
    OfficerInferenceBox* bestBox = nullptr;
    for(int i = 0; i < officerBoxes.size(); i++)
    {
        if(!bestBox || officerBoxes[i].confidence > bestBox->confidence)
        {
            delete bestBox;
            bestBox = new OfficerInferenceBox(officerBoxes[i]);
        }
    }

    // If our best box is still null, we didn't find an officer.
    if(bestBox)
    {
        Log("Highest Confidence: " + to_string(bestBox->confidence), Officers);
    }

    return bestBox;
}
