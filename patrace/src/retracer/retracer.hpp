#ifndef _RETRACER_HPP_
#define _RETRACER_HPP_

#include "retracer/retrace_options.hpp"
#include "retracer/state.hpp"
#include "retracer/texture.hpp"
#include "helper/states.h"
#include "graphic_buffer/GraphicBuffer.hpp"
#include "dma_buffer/dma_buffer.hpp"

#include "common/file_format.hpp"
#include "common/in_file_mt.hpp"
#include "common/os.hpp"
#include "common/os_time.hpp"
#include "common/memory.hpp"
#ifndef _WIN32
#include "common/memoryinfo.hpp"
#endif

#include <atomic>
#include <string>
#include <vector>
#include <deque>
#include <thread>
#include <condition_variable>
#include <unordered_map>
#include <mutex>

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

class OffscreenManager;
class Quad;
class Collection;

namespace retracer {

struct thread_result // internal counters
{
    int our_tid = -1;
    int total = 0;
    int skipped = 0;
    int handovers = 0;
    int wakeups = 0;
    int timeouts = 0;
    int swaps = 0;
};

class Retracer
{
public:
    Retracer() {}
    virtual ~Retracer();

    bool OpenTraceFile(const char* filename);
    void CloseTraceFile();

    void Retrace();
    void RetraceThread(int threadidx, int our_tid);

    void CheckGlError();

    void reportAndAbort(const char *format, ...) NORETURN;
    void saveResult(Json::Value& results);
    bool addResultInformation();

    void OnFrameComplete();
    void OnNewFrame();
    void StartMeasuring();

    void TriggerScript(const char* scriptPath);

    StateLogger& getStateLogger() { return mStateLogger; }

    common::HeaderVersion getFileFormatVersion() const { return mFileFormatVersion; }

    std::string changeAttributesToConstants(const std::string& source, const std::vector<VertexArrayInfo>& attributesToRemove);
    std::vector<Texture> getTexturesToDump();
    void TakeSnapshot(unsigned int callNo, unsigned int frameNo, const char *filename = NULL);
    void StepShot(unsigned int callNo, unsigned int frameNo, const char *filename = NULL);
    void dumpUniformBuffers(unsigned int callno);
    inline int getCurTid() const { return mCurCall.tid; }
    inline unsigned GetCurCallId() const { return mFile.curCallNo; }
    const char* GetCurCallName() const { return mFile.ExIdToName(mCurCall.funcId); }
    inline unsigned GetCurDrawId() const { return mCurDrawNo; }
    inline void IncCurDrawId() { mCurDrawNo++; drawBudget--; }
    inline unsigned GetCurFrameId() const { return mCurFrameNo; }
    inline void IncCurFrameId() { mCurFrameNo++; frameBudget--; }
    inline Context& getCurrentContext() { return *mState.mThreadArr[getCurTid()].getContext(); }
    inline bool hasCurrentContext() const { return mState.mThreadArr[getCurTid()].getContext() != nullptr; }

    void DiscardFramebuffers();
    void PerfStart();
    void PerfEnd();

    common::InFile mFile;
    RetraceOptions mOptions;
    StateMgr mState;
    common::BCall_vlen mCurCall;
    std::atomic_bool mFinish;
    OffscreenManager *mpOffscrMgr = nullptr;
    common::ClientSideBufferObjectSet mCSBuffers;
    Quad *mpQuad = nullptr;

    unsigned mVBODataSize = 0;
    unsigned mTextureDataSize = 0;
    unsigned mCompressedTextureDataSize = 0;
    unsigned mClientSideMemoryDataSize = 0;
    std::unordered_map<std::string, int> mCallCounter;

    Collection *mCollectors = nullptr;

    bool mMosaicNeedToBeFlushed = false;
    bool delayedPerfmonInit = false;
    void perfMonInit();
    int mSurfaceCount = 0;

    struct ProgramCache
    {
        GLenum format;
        std::vector<char> buffer;
    };
    FILE *shaderCacheFile = NULL;
    std::string shaderCacheVersionMD5;
    std::unordered_map<std::string, uint64_t> shaderCacheIndex; // md5 of shader source to offset of cache file
    std::unordered_map<std::string, ProgramCache> shaderCache; // md5 of shader source to cache struct in mem
    int64_t frameBudget = INT64_MAX;
    int64_t drawBudget = INT64_MAX;

    void* fptr = nullptr;
    char* src = nullptr;
    std::deque<std::condition_variable> conditions;
    std::deque<std::thread> threads;
    std::unordered_map<int, int> thread_remapping;
    std::atomic_int latest_call_tid;
    std::mutex mConditionMutex;

private:
    bool loadRetraceOptionsByThreadId(int tid);
    void loadRetraceOptionsFromHeader();
    float getDuration(int64_t lastTime, int64_t* thisTime) const;
    float ticksToSeconds(long long t) const;
    void initializeCallCounter();

#ifndef _WIN32
    bool addMaliRegisterInformation();
#endif

    std::deque<thread_result> results;

    unsigned short mExIdEglSwapBuffers;
    unsigned short mExIdEglSwapBuffersWithDamage;

    int64_t mInitTime = 0;
    int64_t mInitTimeMono = 0;
    int64_t mInitTimeMonoRaw = 0;
    int64_t mInitTimeBoot = 0;
    int64_t mEndFrameTime = 0;
    int64_t mTimerBeginTime = 0;
    int64_t mTimerBeginTimeMono = 0;
    int64_t mTimerBeginTimeMonoRaw = 0;
    int64_t mTimerBeginTimeBoot = 0;
    int64_t mFinishSwapTime = 0;

    StateLogger mStateLogger;
    common::HeaderVersion mFileFormatVersion = common::INVALID_VERSION;
    std::vector<std::string> mSnapshotPaths;

    struct CallStat
    {
        uint64_t count = 0;
        uint64_t time = 0;
    };
    std::unordered_map<std::string, CallStat> mCallStats;

    pid_t child = 0;

    int mLoopTimes = 0;
    std::vector<float> mLoopResults;
    int64_t mLoopBeginTime = 0;

    unsigned mCurDrawNo = 0;
    unsigned mCurFrameNo = 0;
    unsigned mRollbackCallNo = 0;
};

inline float Retracer::getDuration(int64_t lastTime, int64_t* thisTime) const
{
    int64_t now = os::getTime();
    float duration = ticksToSeconds(now - lastTime);
    *thisTime = now;
    return duration;
}

inline float Retracer::ticksToSeconds(long long t) const
{
    float oneOverFreq = 1.0f / os::timeFrequency;
    return t * oneOverFreq;
}

extern Retracer gRetracer;

void pre_glDraw();
void post_glLinkProgram(GLuint shader, GLuint originalShaderName, int status);
void post_glCompileShader(GLuint program, GLuint originalProgramName);
void post_glShaderSource(GLuint shader, GLuint originalshaderName, GLsizei count, const GLchar **string, const GLint *length);
void OpenShaderCacheFile();
bool load_from_shadercache(GLuint program, GLuint originalProgramName, int status);
void hardcode_glBindFramebuffer(int target, unsigned int framebuffer);
void hardcode_glDeleteBuffers(int n, unsigned int* oldBuffers);
void hardcode_glDeleteFramebuffers(int n, unsigned int* oldBuffers);
void hardcode_glDeleteRenderbuffers(int n, unsigned int* oldBuffers);
void hardcode_glDeleteTextures(int n, unsigned int* oldTextures);
void hardcode_glDeleteTransformFeedbacks(int n, unsigned int* oldTransformFeedbacks);
void hardcode_glDeleteQueries(int n, unsigned int* oldQueries);
void hardcode_glDeleteSamplers(int n, unsigned int* oldSamplers);
void hardcode_glDeleteVertexArrays(int n, unsigned int* oldVertexArrays);
void forceRenderMosaicToScreen();

GLuint lookUpPolymorphic(GLuint name, GLenum target);

} /* namespace retracer */

#endif /* _RETRACE_HPP_ */
