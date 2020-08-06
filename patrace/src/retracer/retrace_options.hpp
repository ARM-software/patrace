#pragma once

#include <string>
#include <vector>
#include "retracer/eglconfiginfo.hpp"
#include "common/trace_callset.hpp"

namespace retracer
{

enum Profile
{
    PROFILE_ES1 = 1,
    PROFILE_ES2 = 2,
    PROFILE_ES3 = 3,
    PROFILE_ES31 = 4,
    PROFILE_ES32 = 5,
};

struct RetraceOptions
{
    RetraceOptions() :
          mOnscreenConfig(0, 0, 0, 0, 0, 0, -1, 0)
        , mOffscreenConfig(0, 0, 0, 0, 0, 0, -1, 0)
    {}

    ~RetraceOptions()
    {
        delete mSnapshotCallSet;
        delete mSkipCallSet;
    }

    std::string         mFileName;
    int                 mRetraceTid = -1;
    bool                mForceOffscreen = false;
    bool                mDoOverrideWinSize = false;
    bool                mDoOverrideResolution = false;
    bool                mPreload = false;
    unsigned int        mBeginMeasureFrame = 1;
    unsigned int        mEndMeasureFrame = 9999999;

    int                 mWindowWidth = 0;
    int                 mWindowHeight = 0;
    int                 mOverrideResW = 0;
    int                 mOverrideResH = 0;
    float               mOverrideResRatioW = 0.0f;
    float               mOverrideResRatioH = 0.0f;

    std::string         mSnapshotPrefix;
    common::CallSet*    mSnapshotCallSet = nullptr;
    bool                mUploadSnapshots = false;
    bool                mFailOnShaderError = false;
    int                 mDebug = 0;
    bool                mStateLogging = false;
    common::CallSet*    mSkipCallSet = nullptr;

    EglConfigInfo mOnscreenConfig;
    EglConfigInfo mOffscreenConfig;

    bool                mMeasurePerFrame = false;
    bool                mMeasureSwapTime = false;
    Profile             mApiVersion = PROFILE_ES2;
    bool                mSnapshotFrameNames = false;

    bool                mStrictEGLMode = false;
    bool                mStrictColorMode = false;
    bool                mStoreProgramInformation = false;
    bool                mRemoveUnusedVertexAttributes = false;
    unsigned int        mOnscrSampleW = 48;
    unsigned int        mOnscrSampleH = 27;
    unsigned int        mOnscrSampleNumX = 10;
    unsigned int        mOnscrSampleNumY = 10;

    bool                mForceSingleWindow = false;
    bool                mMultiThread = false;
    int                 mSkipWork = -1;
    bool                mDumpStatic = false;
    bool                mCallStats = false;

    bool                mPbufferRendering = false;

    bool                mFlushWork = false;

    int                 mPerfStart = -1;
    int                 mPerfStop = -1;
    int                 mPerfFreq = 1000;

    bool                mFinishBeforeSwap = false;
    bool                mPerfmon = false;

    std::vector<unsigned int> mLinkErrorWhiteListCallNum;
#if __ANDROID__
    std::string         mPerfPath = "/system/bin/perf";
    std::string         mPerfOut = "/sdcard/perf.data";
#else
    std::string         mPerfPath = "/usr/bin/perf";
    std::string         mPerfOut = "perf.data";
#endif

    std::string         mCpuMask;

    bool                dmaSharedMemory = false;
    std::string         mShaderCacheFile;
    bool                mShaderCacheRequired = false;
private:
    // Noncopyable because of owned CallSet pointer members
    RetraceOptions(const RetraceOptions&);
    RetraceOptions& operator=(const RetraceOptions&);
};

}
