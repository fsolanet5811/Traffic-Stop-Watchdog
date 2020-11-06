#include "io.hpp"
#include <string>
#include <fstream>
#include <streambuf>

using namespace tsw::io;
using namespace std;

Settings::Settings()
{

}

Settings::Settings(string settingsFile)
{
    Load(settingsFile);
    
}

void Settings::Load(string settingsFile)
{
    // Parse the json text.
    ifstream sf(settingsFile, ifstream::binary);
    getline(sf, DeviceSerialPath);
    getline(sf, CameraSerialNumber);
    string officerClassId;
    getline(sf, officerClassId);
    OfficerClassId = stoi(officerClassId);
    sf.close();
}