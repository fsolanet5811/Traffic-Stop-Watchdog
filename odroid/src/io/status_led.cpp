#include "io.hpp"
#include <fstream>

using namespace tsw::io;

StatusLED::StatusLED(string ledFile)
{
    _ledFile = ledFile;
    _isFlashing = false;
    _isEnabled = true;
    FlashesPerPause = 1;
    PauseTime = 750000;
}

void StatusLED::StartFlashing()
{
    StartFlashing(FlashesPerPause);
}

void StatusLED::StartFlashing(int flashesPerPause)
{
    if(IsEnabled() && !IsFlashing())
    {
        _isFlashing = true;
        _flashFuture = async(launch::async, [this]()
        {
            RunFlash();
        });
    }
}

void StatusLED::StopFlashing(bool reset)
{
    if(IsEnabled() && IsFlashing())
    {
        _isFlashing = false;
        
        // Wait for the flashing to actually be done.
        _flashFuture.wait();

        // See if we gotta reset back to off.
        if(reset)
        {
            SetBrightness(LED_OFF);
        }
    }
}

bool StatusLED::IsFlashing()
{
    return _isFlashing;
}

void StatusLED::SetEnabled(bool enabled)
{
    // Disabling this guy should stop flashing.
    if(!enabled)
    {
        StopFlashing();
    }

    _isEnabled = enabled;
}

bool StatusLED::IsEnabled()
{
    return _isEnabled;
}

void StatusLED::RunFlash()
{
    while(IsFlashing())
    {
        // The last flash will not have a small off time.
        Log("Starting flash sequence of " + to_string(FlashesPerPause), LED);
        for(int i = 0; i < FlashesPerPause - 1 && IsFlashing(); i++)
        {
            // First turn on.
            SetBrightness(LED_ON);

            // Wait a bit so the light stays on for a bit.
            usleep(FLASH_ON_TIME);


            if(IsFlashing())
            {
                // Turn off and wait for the next set.
                SetBrightness(LED_OFF);
                usleep(FLASH_OFF_TIME);
            }
        }

        // Break out asap.
        if(!IsFlashing())
        {
            break;
        }

        if(IsFlashing())
        {
            // One last flash on.
            SetBrightness(LED_ON);
            usleep(FLASH_ON_TIME);
        }
        

        // Here it is very important that we double check for the stopped flash.
        if(IsFlashing())
        {
            // Pause before the next set of flashes.
            SetBrightness(LED_OFF);
            usleep(PauseTime);
        }

        Log("Flash sequence finished", LED);
    }
}

void StatusLED::SetBrightness(unsigned char brightness)
{
    // Open a stream to the led brightness file.
    // Opening with the trunc option clears it every time we open it.
    fstream ledFileStream;
    ledFileStream.open(_ledFile, fstream::out | fstream::trunc);

    // The value is written to the file as plain text.
    string brightnessStr = to_string(brightness);
    ledFileStream << brightnessStr;
    Log("LED set to " + brightnessStr, LED);

    // Release the lock on the file.
    ledFileStream.close();
}
