
#include "fps_log.hpp"

#include "../../common/os_time.hpp"

using namespace os;

FpsLog gFpsLog(10, 1.0f);

FpsLog::FpsLog(int frameCnt, float updateFreq)
    : mFrameCnt(frameCnt),
    mUpdateFreq(updateFreq),
    mNextFrameSlot(0),
    mValidCnt(0)
{
    mFrameLens = new long long [mFrameCnt];
    mFrameBeg = getTime();
    mProcessBeg = mFrameBeg;
}

FpsLog::~FpsLog()
{
    delete [] mFrameLens;
}

void FpsLog::SwapBufferHappens()
{
    long long frameEnd = getTime();
    mFrameLens[mNextFrameSlot] = frameEnd - mFrameBeg;
    
    mFrameBeg = frameEnd;
    mNextFrameSlot = (mNextFrameSlot+1) % mFrameCnt;
    if (mValidCnt < mFrameCnt)
        mValidCnt++;

    // show fps
    static float lastUpdate = 0.0f;
    static float oneOverFreq = 1.0f / timeFrequency;

    float curTime = (frameEnd-mProcessBeg)*oneOverFreq;
    if (curTime - lastUpdate > mUpdateFreq)
    {
        lastUpdate = curTime;
        
        long long durationSum = 0;
        for (int i = 0; i < mValidCnt; ++i)
            durationSum += mFrameLens[i];
        float fps = mValidCnt / (durationSum*oneOverFreq);
        DBG_LOG("FPS: %f\n", fps);
    }
}

