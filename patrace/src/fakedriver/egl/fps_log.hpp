#ifndef _FAKE_DRIVER_FPS_LOG_HPP_
#define _FAKE_DRIVER_FPS_LOG_HPP_

#include "../common.h"

class FpsLog
{
public:
    // <param name = frameCnt>
    // Output the average fps of last 'frameCnt' frames.
    // </param>
    // 
    // <param name = updateFreq>
    // For each 'updateFreq' time, the fps value is updated in the console.
    // </param>
    FpsLog(int frameCnt, float updateFreq);
    
    ~FpsLog();

    void SwapBufferHappens();

private:
    int             mFrameCnt;
    float           mUpdateFreq;
    
    long long*      mFrameLens;
    int             mNextFrameSlot;
    int             mValidCnt;

    long long       mProcessBeg;
    long long       mFrameBeg;
};

extern FpsLog gFpsLog;

#endif
