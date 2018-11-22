#include <tracer/interactivecmd.hpp>
#include <common/os.hpp>

#include <sstream>
#include <fstream>
#include <string>
#include <algorithm>
#include <stdlib.h>

using namespace std;

// trim from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

// Define static member
const char* InteractiveCmd::names[] = {
    "UNKNOWN_CMD",
    "TO_FRAME",        // run until we hit a given frame
    "SNAP_FRAME",      // capture the final framebuffer image
    "SNAP_DRAW_FRAME", // capture all draw calls for a frame
    "SNAP_ALL_FRAMES_NOWAIT" // snap every N frames
};


InteractiveCmd GetInteractiveCmd()
{
    InteractiveCmd ret;

#ifdef ANDROID
    const char* cmdFileName = "/system/lib/egl/tracercmd.cfg";
#else
    const char* cmdFileName = "tracercmd.cfg";
#endif

    std::ifstream file;
    file.open(cmdFileName);

    if (file.is_open())
    {
        string line;
        getline(file, line);

        size_t pos = line.find_first_of(" \t");
        if (pos != string::npos)
        {
            string strCmd = line.substr(0, pos);
            trim(strCmd);

            if(strCmd.compare("snapEvery") == 0) {
                std::stringstream ss(line);
                std::string  strCmd;
                unsigned int tid;
                unsigned int frameRate;
                ss >> strCmd;
                ss >> tid;
                ss >> frameRate;

                ret.cmd = SNAP_ALL_FRAMES_NOWAIT;
                ret.nameCString = ret.names[SNAP_ALL_FRAMES_NOWAIT];
                ret.requiredTid = tid;
                ret.frameRate = frameRate;
            } else {
                // Commands that only take 1 param: frame number
                string strFrNo = line.substr(pos);
                trim(strFrNo);

                if (strCmd.compare("toFr") == 0) {
                    ret.cmd = TO_FRAME;
                    ret.nameCString = ret.names[TO_FRAME];
                    ret.frameNo = atoi(strFrNo.c_str());
                } else if (strCmd.compare("snapFinalFr") == 0) {
                    ret.cmd = SNAP_FRAME;
                    ret.nameCString = ret.names[SNAP_FRAME];
                    ret.frameNo = atoi(strFrNo.c_str());
                } else if (strCmd.compare("snapAllDrawFr") == 0) {
                    ret.cmd = SNAP_DRAW_FRAME;
                    ret.nameCString = ret.names[SNAP_DRAW_FRAME];
                    ret.frameNo = atoi(strFrNo.c_str());
                }
            }

        }
    }

    return ret;
}

