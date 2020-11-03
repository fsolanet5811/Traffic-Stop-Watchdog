#include "io.hpp"
//#include "json.h"
#include <string>
#include <fstream>
#include <streambuf>

using namespace tsw::io;
//using namespace Json;
using namespace std;

void Settings::Load(string fileName)
{
    // Parse the json text.
    //ifstream t(fileName);
    //Reader reader;
    //Value root;
    //reader.parse(t, root);

    //DeviceSerialPath = root["DeviceSerialPath"].asString();
}