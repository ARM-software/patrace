#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "retracer/retracer.hpp"

#include "retracer/glstate.hpp"
#include "retracer/retrace_api.hpp"
#include "retracer/trace_executor.hpp"
#include "retracer/forceoffscreen/offscrmgr.h"
#include "retracer/glws.hpp"
#include "retracer/cmd_options.hpp"
#include "retracer/config.hpp"

#include "libcollector/interface.hpp"

#include "helper/states.h"
#include "helper/shadermod.hpp"

#include "dispatch/eglproc_auto.hpp"

#include "common/image.hpp"
#include "common/os_string.hpp"
#include "common/pa_exception.h"
#include "common/gl_extension_supported.hpp"

#include "hwcpipe/hwcpipe.h"

#include "jsoncpp/include/json/writer.h"
#include "jsoncpp/include/json/reader.h"

#include <errno.h>
#include <algorithm> // for std::min/max
#include <string>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h> // basename
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sched.h>

using namespace common;
using namespace os;
using namespace glstate;
using namespace image;

#include <fstream>

using namespace std;

void retrace_eglMakeCurrent(char* src);
void retrace_eglCreateContext(char* src);
void retrace_eglCreateWindowSurface(char* src);

namespace retracer {

Retracer gRetracer;

static inline uint64_t gettime()
{
    struct timespec t;
    // CLOCK_MONOTONIC_COARSE is much more light-weight, but resolution is quite poor.
    // CLOCK_PROCESS_CPUTIME_ID is another possibility, it ignores rest of system, but costs more,
    // and also on some CPUs process migration between cores can screw up such measurements.
    // CLOCK_MONOTONIC is therefore a reasonable and portable compromise.
    clock_gettime(CLOCK_MONOTONIC, &t);
    return ((uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec);
}

/// -- Mali HWCPipe support

struct mali_hwc
{
	Json::Value::Int64 cpu_cycles;
	Json::Value::Int64 cpu_instructions;
	Json::Value::Int64 gpu_cycles;
	Json::Value::Int64 fragment_cycles;
	Json::Value::Int64 vertex_cycles;
	Json::Value::Int64 read_bw;
	Json::Value::Int64 write_bw;
	Json::Value::Int64 vertex_jobs;
	Json::Value::Int64 fragment_jobs;
} mali_samples;
static hwcpipe::HWCPipe *hwcpipe = nullptr;
static bool is_mali = false;
static bool mali_perf_started = false;

static void mali_perf_init(const char *vendor)
{
    mali_samples = {};
    if (strncasecmp(vendor, "arm", 3) == 0) // was "ARM" but name should now be "Arm"
    {
        is_mali = true;
    }
    hwcpipe::CpuCounterSet cpu{
        hwcpipe::CpuCounter::Cycles,
        hwcpipe::CpuCounter::Instructions,
    };
    hwcpipe::GpuCounterSet mali_gpu{
        hwcpipe::GpuCounter::GpuCycles,
        hwcpipe::GpuCounter::VertexComputeCycles,
        hwcpipe::GpuCounter::FragmentCycles,
        hwcpipe::GpuCounter::Pixels,
        hwcpipe::GpuCounter::ExternalMemoryReadBytes,
        hwcpipe::GpuCounter::ExternalMemoryWriteBytes,
        hwcpipe::GpuCounter::VertexComputeJobs,
        hwcpipe::GpuCounter::FragmentJobs,
    };
    if (is_mali)
    {
        hwcpipe = new hwcpipe::HWCPipe(cpu, mali_gpu);
    }
    else
    {
        hwcpipe = new hwcpipe::HWCPipe(cpu, hwcpipe::GpuCounterSet{});
    }
}

static void mali_perf_run()
{
    hwcpipe->run();
    mali_perf_started = true;
}

static void mali_perf_sample()
{
    if (!mali_perf_started) return;
    const hwcpipe::Measurements& m = hwcpipe->sample();
    if (m.cpu)
    {
        for (const auto& pair : *m.cpu)
        {
            const Json::Value::Int64 value = pair.second.get<int64_t>();
            if (pair.first == hwcpipe::CpuCounter::Cycles) mali_samples.cpu_cycles += value;
            else if (pair.first == hwcpipe::CpuCounter::Instructions) mali_samples.cpu_instructions += value;
        }
    }
    if (m.gpu)
    {
        for (const auto& pair : *m.gpu)
        {
            const Json::Value::Int64 value = pair.second.get<int64_t>();
            switch (pair.first)
            {
            case hwcpipe::GpuCounter::GpuCycles: mali_samples.gpu_cycles += value; break;
            case hwcpipe::GpuCounter::FragmentCycles: mali_samples.fragment_cycles += value; break;
            case hwcpipe::GpuCounter::VertexComputeCycles : mali_samples.vertex_cycles += value; break;
            case hwcpipe::GpuCounter::ExternalMemoryReadBytes : mali_samples.read_bw += value; break;
            case hwcpipe::GpuCounter::ExternalMemoryWriteBytes : mali_samples.write_bw += value; break;
            case hwcpipe::GpuCounter::VertexComputeJobs : mali_samples.vertex_jobs += value; break;
            case hwcpipe::GpuCounter::FragmentJobs : mali_samples.fragment_jobs += value; break;
            default: break; // ignore
            }
        }
    }
}

static void mali_perf_end(Json::Value& v)
{
    if (mali_perf_started)
    {
        hwcpipe->stop();
    }
    mali_perf_started = false;
    delete hwcpipe;
    hwcpipe = nullptr;
    if (mali_samples.cpu_cycles) v["cpu_cycles"] = mali_samples.cpu_cycles;
    if (mali_samples.cpu_instructions) v["cpu_instructions"] = mali_samples.cpu_instructions;
    if (mali_samples.gpu_cycles) v["gpu_active"] = mali_samples.gpu_cycles;
    if (mali_samples.fragment_cycles) v["fragment_active"] = mali_samples.fragment_cycles;
    if (mali_samples.vertex_cycles) v["vertex_active"] = mali_samples.vertex_cycles;
    if (mali_samples.read_bw) v["read_bytes"] = mali_samples.read_bw;
    if (mali_samples.write_bw) v["write_bytes"] = mali_samples.write_bw;
    if (mali_samples.vertex_jobs) v["vertex_jobs"] = mali_samples.vertex_jobs;
    if (mali_samples.fragment_jobs) v["fragment_jobs"] = mali_samples.fragment_jobs;
}

/// --- AMD performance counter extension support

#define GL_COUNTER_TYPE_AMD               0x8BC0
#define GL_COUNTER_RANGE_AMD              0x8BC1
#define GL_UNSIGNED_INT64_AMD             0x8BC2
#define GL_PERCENTAGE_AMD                 0x8BC3
#define GL_PERFMON_RESULT_AVAILABLE_AMD   0x8BC4
#define GL_PERFMON_RESULT_SIZE_AMD        0x8BC5
#define GL_PERFMON_RESULT_AMD             0x8BC6

static GLuint pm_monitor = 0;
static bool amd_perf_activated = false;
#ifdef ANDROID
const char *perfmon_filename = "/sdcard/perfmon.csv";
const char *perfmon_counters = "/sdcard/perfmon_counters.csv";
const char *perfmon_settings = "/sdcard/perfmon.conf";
#else
const char *perfmon_filename = "perfmon.csv";
const char *perfmon_counters = "perfmon_counters.csv";
const char *perfmon_settings = "perfmon.conf";
#endif

static void perfmon_list()
{
    (void)_glGetError(); // reset state
    GLint numGroups = 0;
    _glGetPerfMonitorGroupsAMD(&numGroups, 0, NULL);
    std::vector<GLuint> groups(numGroups);
    FILE* pm_fp = fopen(perfmon_counters, "w");
    if (!pm_fp)
    {
        DBG_LOG("Could not open file for writing perfmon counter list: %s\n", strerror(errno));
        return;
    }
    fprintf(pm_fp, "Group Idx,Group ID,Counter Idx,CounterID,Group Name,Counter Name,Type\n");
    _glGetPerfMonitorGroupsAMD(NULL, numGroups, groups.data());
    bool ok = (_glGetError() == GL_NO_ERROR);
    if (!ok) { DBG_LOG("list: Failed to enumerate groups\n"); }
    for (int i = 0 ; ok && i < numGroups; i++)
    {
        GLint numCounters = 0;
        GLint maxActiveCounters = 0;
        char curGroupName[256];
        memset(curGroupName, 0, sizeof(curGroupName));
        _glGetPerfMonitorGroupStringAMD(groups[i], 256, NULL, curGroupName);
        ok = (_glGetError() == GL_NO_ERROR);
        if (!ok) { DBG_LOG("list: glGetPerfMonitorGroupStringAMD failed\n"); continue; }
        _glGetPerfMonitorCountersAMD(groups[i], &numCounters, &maxActiveCounters, 0, NULL);
        ok = (_glGetError() == GL_NO_ERROR);
        if (!ok) { DBG_LOG("list: glGetPerfMonitorCountersAMD for %s get count failed\n", curGroupName); continue; }
        std::vector<GLuint> counterList(numCounters);
        _glGetPerfMonitorCountersAMD(groups[i], NULL, NULL, numCounters, counterList.data());
        ok = (_glGetError() == GL_NO_ERROR);
        if (!ok) { DBG_LOG("list: glGetPerfMonitorCountersAMD for %s get data failed\n", curGroupName); continue; }
        for (int j = 0; j < numCounters; j++)
        {
            char curCounterName[256];
            memset(curCounterName, 0, sizeof(curCounterName));
            _glGetPerfMonitorCounterStringAMD(groups[i], counterList[j], 256, NULL, curCounterName);
            ok = (_glGetError() == GL_NO_ERROR);
            if (!ok) { DBG_LOG("list: glGetPerfMonitorCounterStringAMD failed\n"); break; }
            GLenum type;
            _glGetPerfMonitorCounterInfoAMD(groups[i], counterList[j], GL_COUNTER_TYPE_AMD, &type);
            fprintf(pm_fp, "%d,%u,%d,%u,%s,%s,0x%04x\n", i, groups[i], j, counterList[j], curGroupName, curCounterName, type);
        }
    }
    fclose(pm_fp);
}

static void amd_perf_init(const char *vendor)
{
    unsigned pm_current_group = 0;
    std::vector<GLuint> perfset;
    amd_perf_activated = false;
    if (!isGlesExtensionSupported("GL_AMD_performance_monitor") && strncasecmp(vendor, "qualcomm", 8) != 0)
    {
        return;
    }
    perfmon_list();
    FILE* pm_fp = fopen(perfmon_settings, "r");
    if (pm_fp)
    {
        int r = fscanf(pm_fp, "%u\n", &pm_current_group);
        if (r != 1)
        {
            DBG_LOG("Failed to read perfmon.conf! No perf measurements for you :-(\n");
            return;
        }
        DBG_LOG("GL_AMD_performance_monitor activating for group %u and counters:", pm_current_group);
        unsigned v = 0;
        do // read counter values
        {
            r = fscanf(pm_fp, "%u\n", &v);
            if (r == 1)
            {
                perfset.push_back(v);
            }
        }
        while (r == 1);
        fclose(pm_fp);
        pm_fp = NULL;
    }
    else // just get top level gpu cycles
    {
        DBG_LOG("Failed to open perfmon.conf - using default counter info instead\n");
        perfset.push_back(2);
    }
    (void)_glGetError();
    GLint numGroups = 0;
    std::vector<GLuint> groups;
    _glGetPerfMonitorGroupsAMD(&numGroups, 0, NULL);
    groups.resize(numGroups);
    _glGetPerfMonitorGroupsAMD(NULL, numGroups, groups.data());
    GLint numCounters = 0;
    GLint maxActiveCounters = 0;
    _glGetPerfMonitorCountersAMD(groups[pm_current_group], &numCounters, &maxActiveCounters, 0, NULL);
    bool ok = (_glGetError() == GL_NO_ERROR);
    if (!ok)
    {
        DBG_LOG("glGetPerfMonitorCountersAMD failed to get counter stats\n");
        return;
    }
    if ((unsigned)maxActiveCounters < perfset.size())
    {
        DBG_LOG("Too many counters defined (%d max, trying to use %u) - concatenating the list\n", maxActiveCounters, (unsigned)perfset.size());
        perfset.resize(maxActiveCounters);
    }
    std::vector<GLuint> counterList(numCounters);
    _glGetPerfMonitorCountersAMD(groups[pm_current_group], NULL, NULL, numCounters, counterList.data());
    ok = (_glGetError() == GL_NO_ERROR);
    if (!ok)
    {
        DBG_LOG("glGetPerfMonitorCountersAMD failed to get counter list\n");
        _glDeletePerfMonitorsAMD(1, &pm_monitor);
        return;
    }
    _glGenPerfMonitorsAMD(1, &pm_monitor);
    std::vector<GLuint> active_counters(perfset.size());
    for (unsigned i = 0; i < perfset.size(); i++)
    {
         active_counters[i] = counterList[perfset[i]];
    }
    _glSelectPerfMonitorCountersAMD(pm_monitor, GL_TRUE, groups[pm_current_group], perfset.size(), active_counters.data());
    ok = (_glGetError() == GL_NO_ERROR);
    DBG_LOG("Enabling perf monitor for group %u: %s\n", groups[pm_current_group], ok ? "OK" : "FAILED");
    if (!ok)
    {
        DBG_LOG("Failed to enable perf monitor!\n");
        _glDeletePerfMonitorsAMD(1, &pm_monitor);
        return;
    }
    // Write out csv header if file does not exist already
    pm_fp = fopen(perfmon_filename, "w");
    if (!pm_fp)
    {
        DBG_LOG("Could not open file %s for writing perfmon header: %s\n", perfmon_filename, strerror(errno));
        _glDeletePerfMonitorsAMD(1, &pm_monitor);
        return;
    }
    char groupname[256];
    char name[256];
    fprintf(pm_fp, "Name");
    _glGetPerfMonitorGroupStringAMD(groups[pm_current_group], 256, NULL, groupname);
    for (unsigned j = 0; j < perfset.size(); j++)
    {
        _glGetPerfMonitorCounterStringAMD(groups[pm_current_group], active_counters[j], 256, NULL, name);
        fprintf(pm_fp, ",%s:%s", groupname, name);
    }
    fprintf(pm_fp, "\n");
    fclose(pm_fp);
    amd_perf_activated = true;
}

static void amd_perf_run()
{
    if (!amd_perf_activated)
    {
        return;
    }
    _glBeginPerfMonitorAMD(pm_monitor);
    bool ok = (_glGetError() == GL_NO_ERROR);
    if (!ok)
    {
        DBG_LOG("perfmon : FAILED at begin\n");
        _glDeletePerfMonitorsAMD(1, &pm_monitor);
        amd_perf_activated = false;
        return;
    }
}

static void amd_perf_end(Json::Value& v)
{
    if (!amd_perf_activated)
    {
        return;
    }
    _glEndPerfMonitorAMD(pm_monitor);
    bool ok = (_glGetError() == GL_NO_ERROR);
    if (!ok)
    {
        DBG_LOG("perfmon end : FAILED\n");
    }
    FILE* pm_fp = fopen(perfmon_filename, "a");
    if (!pm_fp)
    {
        DBG_LOG("Could not open file %s for writing perfmon data: %s\n", perfmon_filename, strerror(errno));
    }
    GLuint dataAvail = GL_FALSE;
    int repeat = 0;
    do
    {
        _glGetPerfMonitorCounterDataAMD(pm_monitor, GL_PERFMON_RESULT_AVAILABLE_AMD, sizeof(GLuint), &dataAvail, NULL);
        if (dataAvail == GL_FALSE)
        {
            _glFinish();
            usleep(1000);
        }
        repeat++;
    }
    while (dataAvail == GL_FALSE && repeat < 20);
    if (!dataAvail) DBG_LOG("Failed to find any perfmon data!\n");
    GLsizei written = 0;
    GLuint resultSize = 0;
    _glGetPerfMonitorCounterDataAMD(pm_monitor, GL_PERFMON_RESULT_SIZE_AMD, sizeof(GLuint), &resultSize, NULL);
    ok = (_glGetError() == GL_NO_ERROR);
    if (!ok) DBG_LOG("perfmon end : Could not fetch data size\n");
    std::vector<GLuint> result(resultSize / sizeof(GLuint));
    _glGetPerfMonitorCounterDataAMD(pm_monitor, GL_PERFMON_RESULT_AMD, resultSize, (GLuint*)result.data(), &written);
    ok = (_glGetError() == GL_NO_ERROR);
    if (!ok) DBG_LOG("perfmon end : Could not fetch data itself\n");
    fprintf(pm_fp, "run");
    GLsizei wordCount = 0;
    while (wordCount * (GLsizei)sizeof(GLuint) < written)
    {
        GLuint groupId = result[wordCount];
        GLuint counterId = result[wordCount + 1];
        uint64_t counterResult = 0;
        memcpy(&counterResult, &result[wordCount + 2], sizeof(uint64_t));
        if (pm_fp) fprintf(pm_fp, ",%llu", (unsigned long long)counterResult);
        if (groupId == 0 && counterId == 2) v["gpu_active"] = (Json::Value::Int64)counterResult;
        wordCount += 4;
    }
    fprintf(pm_fp, "\n");
    fclose(pm_fp);
    _glDeletePerfMonitorsAMD(1, &pm_monitor);
    amd_perf_activated = false;
    DBG_LOG("Perfmon data successfully written (%d words, %u result size)\n", (int)wordCount, (unsigned)resultSize);
}

/// --- Performance monitoring

static void perfmon_init()
{
    const char *vendor = (const char *)_glGetString(GL_VENDOR);
    mali_perf_init(vendor);
    amd_perf_init(vendor);

    mali_perf_run();
    amd_perf_run();
}

static void perfmon_frame()
{
    mali_perf_sample();
}

static void perfmon_end(Json::Value& v)
{
    mali_perf_end(v);
    amd_perf_end(v);
}

/// ---

WorkThread::WorkThread(InFile *f) :
    file(f),
    workQueue(MAX_WORK_QUEUE_SIZE),
    curWork(NULL)
{
    setStatus(UNKNOWN);
}

WorkThread::~WorkThread()
{
}

void WorkThread::workQueueWakeup()
{
    workQueue.wakeup();
}

void WorkThread::workQueuePush(Work *work)
{
    if (work)
    {
        setStatus(RUNNING);
        work->setStatus(Work::SUBMITED);
        work->retain();
        if (work->getChunkHandler())
            work->getChunkHandler()->retain();
        while(!workQueue.push(work));
    }
}

WorkThread::WorkThreadStatus WorkThread::getStatus()
{
    WorkThreadStatus s = UNKNOWN;
    _access();
    s = status;
    _exit();
    return s;
}

void WorkThread::setStatus(WorkThread::WorkThreadStatus s)
{
    _access();
    status = s;
    _exit();
}

void WorkThread::run()
{
    setStatus(IDLE);
    workThreadCond.inc();
    workThreadCond.broadcast();
    while (getStatus() != TERMINATED)
    {
        workQueue.waitNewItem();
        workThreadCond.dec();
        while (!workQueue.isEmpty())
        {
            curWork = workQueuePop();
            if (curWork == NULL) break;
            curWork->run();
            getFileHandler()->releaseChunk(curWork->getChunkHandler());
            curWork->release();

            // Error Check
            if (gRetracer.mOptions.mDebug && gRetracer.hasCurrentContext())
            {
                gRetracer.CheckGlError();
            }
        }
        curWork = NULL;
        setStatus(IDLE);
        workThreadCond.inc();
        workThreadCond.broadcast();
    }
    workThreadCond.dec();
    setStatus(END);
}

void WorkThread::waitIdle()
{
    while (getStatus() == RUNNING)
    {
        workThreadCond.wait();
    }
}

void WorkThread::terminate()
{
    setStatus(TERMINATED);
}

Work::Work(int tid, unsigned frameId, unsigned callId, void* fptr, char* src, const char* name, UnCompressedChunk *chunk)
{
    _tid     = tid;
    _frameId = frameId;
    _callId  = callId;
    _fptr    = fptr;
    _src     = src;
    _callName= name;
    _chunk   = chunk;
    ref      = 0;
    start_time = 0;
    end_time   = 0;
    isMeasureTime = false;
    setStatus(CREATED);
}

Work::~Work()
{
}

int Work::getThreadID()
{
    return _tid;
}

unsigned Work::getFrameID()
{
    return _frameId;
}

unsigned Work::getCallID()
{
    return _callId;
}

void Work::run()
{
    setStatus(EXECUTED);
    (*(RetraceFunc)_fptr)(_src);
    onFinished();
}

void Work::onFinished()
{
    setStatus(COMPLETED);
    if (isMeasureTime)
    {
        gRetracer.UpdateCallStats(_callName, GetExecutionTime());
    }
}

void Work::setStatus(Work::WorkStatus s)
{
    // This function can be used to calculate the execute time of each work stage in the future.
    workMutex.lock();
    status = s;
    if (isMeasureTime)
    {
        switch (s)
        {
            case EXECUTED:
                start_time = gettime();
                break;
            case COMPLETED:
                end_time = gettime();
                break;
            default:
                break;
        }
    }
    workMutex.unlock();
}

Work::WorkStatus Work::getStatus()
{
    WorkStatus s = UNKNOWN;
    workMutex.lock();
    s = status;
    workMutex.unlock();
    return s;
}

void Work::release()
{
    unsigned left = 0;
    workMutex.lock();
    left = --ref;
    workMutex.unlock();

    if (left == 0)
    {
        delete this;
    }
}

void Work::retain()
{
    workMutex.lock();
    ref++;
    workMutex.unlock();
}

int  Work::getRefCount()
{
    unsigned left = 0;
    workMutex.lock();
    left = ref;
    workMutex.unlock();

    return left;
}

void Work::dump()
{
    DBG_LOG("Call(%s): tid %d, frameId %d, callId %d\n", _callName, _tid, _frameId, _callId);
}

void SnapshotWork::run()
{
    gRetracer.TakeSnapshot(getCallID(), getFrameID());
}

void FlushWork::run()
{
    gRetracer.Flush();
}

void DiscardFramebuffersWork::run()
{
    gRetracer.DiscardFramebuffers();
}

void PerfWork::run()
{
    if (strcmp(GetCallName(), "PerfStart") == 0)
    {
        gRetracer.PerfStart();
    }
    else if (strcmp(GetCallName(), "PerfEnd") == 0)
    {
        gRetracer.PerfEnd();
    }
}

void StepWork::run()
{
    gRetracer.StepShot(getCallID(), getFrameID());
}

Retracer::Retracer()
 : mFile()
 , mOptions()
 , mState()
 , mCurCall()
 , mOutOfMemory(false)
 , mFailedToLinkShaderProgram(false)
 , mFinish(false)
 , mpOffscrMgr(NULL)
 , mCSBuffers()
 , mpQuad(NULL)
 , mVBODataSize(0)
 , mTextureDataSize(0)
 , mCompressedTextureDataSize(0)
 , mClientSideMemoryDataSize(0)
 , mCallCounter()
 , mCollectors(NULL)
 , mExIdEglSwapBuffers(0)
 , mExIdEglSwapBuffersWithDamage(0)
 , mStateLogger()
 , mFileFormatVersion(INVALID_VERSION)
 , mSnapshotPaths()
{
#ifndef NDEBUG
    mVBODataSize = 0;
    mTextureDataSize = 0;
    mCompressedTextureDataSize = 0;
    mClientSideMemoryDataSize = 0;
#endif

    staticDump["indirect"] = Json::arrayValue;
    staticDump["uniforms"] = Json::arrayValue;

    createWorkThreadPool();
}

Retracer::~Retracer()
{
    wakeupAllWorkThreads();
    waitWorkThreadPoolIdle();

    if (mOptions.mDumpStatic)
    {
        Json::StyledWriter writer;
        std::string data = writer.write(staticDump);

        FILE *fp = fopen("static.json", "w");
        if (fp)
        {
            size_t written;
            do
            {
                written = fwrite(data.c_str(), data.size(), 1, fp);
            } while (!ferror(fp) && !written);
            if (ferror(fp))
            {
                DBG_LOG("Failed to write output JSON: %s\n", strerror(errno));
            }
            fsync(fileno(fp));
            fclose(fp);
        }
        else
        {
            DBG_LOG("Failed to open output JSON: %s\n", strerror(errno));
        }
    }

    delete mCollectors;

#ifndef NDEBUG
    if(mVBODataSize)
        DBG_LOG("VBO data size : %d\n", mVBODataSize);
    if(mTextureDataSize)
        DBG_LOG("Uncompressed texture data size : %d\n", mTextureDataSize);
    if(mCompressedTextureDataSize)
        DBG_LOG("Compressed texture data size : %d\n", mCompressedTextureDataSize);
    if(mClientSideMemoryDataSize)
        DBG_LOG("Client-side memory data size : %d\n", mClientSideMemoryDataSize);
#endif
}

bool Retracer::OpenTraceFile(const char* filename)
{
    mCurCallNo = 0;
    mCurDrawNo = 0;
    mCurFrameNo = 0;
    mDispatchFrameNo = 0;
    mOutOfMemory = false;
    mFailedToLinkShaderProgram = false;
    mFinish = false;
    mExIdEglSwapBuffers = 0;
    mExIdEglSwapBuffersWithDamage = 0;

    mTimerBeginTime = 0;
    mEndFrameTime = 0;

    createWorkThreadPool();
    mFile.prepareChunks();
    if (!mFile.Open(filename))
        return false;

    mFileFormatVersion = mFile.getHeaderVersion();

    mStateLogger.open(std::string(filename) + ".retracelog");

    loadRetraceOptionsFromHeader();

    mExIdEglSwapBuffers = mFile.NameToExId("eglSwapBuffers");
    mExIdEglSwapBuffersWithDamage = mFile.NameToExId("eglSwapBuffersWithDamageKHR");

    initializeCallCounter();

    staticDump["source"] = filename;

    return true;
}

__attribute__ ((noinline)) static int noop(int a)
{
    return a + 1;
}

void Retracer::CloseTraceFile() {
    mFile.Close();
    mFileFormatVersion = INVALID_VERSION;
    mStateLogger.close();

    if (mOptions.mCallStats)
    {
        // First generate some info on no-op calls as a baseline
        int c = 0;
        for (int i = 0; i < 1000; i++)
        {
            auto pre = gettime();
            c = noop(c);
            auto post = gettime();
            mCallStats["NO-OP"].count++;
            mCallStats["NO-OP"].time += post - pre;
            usleep(c); // just to use c for something, to make 100% sure it is not optimized away
        }
#if ANDROID
        const char *filename = "/sdcard/callstats.csv";
#else
        const char *filename = "callstats.csv";
#endif
        DBG_LOG("Writing callstats to %s\n", filename);
        FILE *fp = fopen(filename, "w");
        if (fp)
        {
            fprintf(fp, "Function,Calls,Time\n");
            for (const auto& pair : mCallStats)
            {
                fprintf(fp, "%s,%" PRIu64 ",%" PRIu64 "\n", pair.first.c_str(), pair.second.count, pair.second.time);
            }
            fsync(fileno(fp));
            fclose(fp);
        }
        else
        {
            DBG_LOG("Failed to open output callstats in %s: %s\n", filename, strerror(errno));
        }
        mCallStats.clear();
    }

    mState.Reset();
    mCSBuffers.clear();
    mSnapshotPaths.clear();
}

bool Retracer::loadRetraceOptionsByThreadId(int tid)
{
    const Json::Value jsThread = mFile.getJSONThreadById(tid);
    if ( jsThread.isNull() ) {
        DBG_LOG("No stored EGL config for this tid: %d\n", tid);
        return false;
    }
    mOptions.mWindowWidth = jsThread["winW"].asInt();
    mOptions.mWindowHeight = jsThread["winH"].asInt();

    mOptions.mOnscreenConfig = jsThread["EGLConfig"];
    mOptions.mOffscreenConfig = jsThread["EGLConfig"];

    return true;
}

void Retracer::loadRetraceOptionsFromHeader()
{
    // Load values from headers first, then any valid commandline parameters override the header defaults.
    const Json::Value jsHeader = mFile.getJSONHeader();
    mOptions.mRetraceTid = jsHeader.get("defaultTid", -1).asInt();
    mOptions.mForceSingleWindow = jsHeader.get("forceSingleWindow", false).asBool();
    if (mOptions.mForceSingleWindow)
    {
        DBG_LOG("Enabling force single window option\n");
    }
    mOptions.mMultiThread = jsHeader.get("multiThread", false).asBool();
    if (mOptions.mMultiThread)
    {
        DBG_LOG("Enabling multiple thread option\n");
    }
    switch (jsHeader["glesVersion"].asInt())
    {
    case 1: mOptions.mApiVersion = PROFILE_ES1; break;
    case 2: mOptions.mApiVersion = PROFILE_ES2; break;
    case 3: mOptions.mApiVersion = PROFILE_ES3; break;
    default: DBG_LOG("Error: Invalid glesVersion parameter\n"); break;
    }
    loadRetraceOptionsByThreadId( mOptions.mRetraceTid );
    const Json::Value& linkErrorWhiteListCallNum = jsHeader["linkErrorWhiteListCallNum"];
    for(unsigned int i=0; i<linkErrorWhiteListCallNum.size(); i++)
    {
        mOptions.mLinkErrorWhiteListCallNum.push_back(linkErrorWhiteListCallNum[i].asUInt());
    }
}

// FIXME - Lots of code duplication here with TraceExecutor::overrideDefaultsWithJson()
bool Retracer::overrideWithCmdOptions( const CmdOptions &cmdOptions )
{
    mOptions.mDebug = cmdOptions.debug;
    mOptions.mStateLogging = cmdOptions.stateLogging;
    if (cmdOptions.skipWork >= 0)
    {
        mOptions.mSkipWork = cmdOptions.skipWork;
    }
    mOptions.mDumpStatic = cmdOptions.dumpStatic;
    mOptions.mCallStats = cmdOptions.callStats;

    if (cmdOptions.finishBeforeSwap)
    {
        mOptions.mFinishBeforeSwap = true;
    }

    if (!cmdOptions.cpuMask.empty()) mOptions.mCpuMask = cmdOptions.cpuMask;

    if (cmdOptions.perfStart != -1)
    {
        mOptions.mPerfStart = cmdOptions.perfStart;
        mOptions.mPerfStop = cmdOptions.perfStop;
    }
    if (cmdOptions.perfFreq != -1)
    {
        mOptions.mPerfFreq = cmdOptions.perfFreq;
    }
    if (!cmdOptions.perfPath.empty())
    {
        mOptions.mPerfPath = cmdOptions.perfPath;
    }
    if (!cmdOptions.perfOut.empty())
    {
        mOptions.mPerfOut = cmdOptions.perfOut;
    }

    mOptions.mSnapshotFrameNames = cmdOptions.snapshotFrameNames;
    mOptions.mPbufferRendering = cmdOptions.pbufferRendering;

    if (cmdOptions.tid != -1 && cmdOptions.tid != mOptions.mRetraceTid)
    {
        if (cmdOptions.tid >= PATRACE_THREAD_LIMIT)
        {
            DBG_LOG("Error: thread IDs only up to %d supported (configured thread ID %d).\n",
                    PATRACE_THREAD_LIMIT, cmdOptions.tid);
            return false;
        }

        mOptions.mRetraceTid = cmdOptions.tid;
        DBG_LOG("Overriding default tid with: %d\n", cmdOptions.tid);

        // Since tid is different from header default, load options that depend on tid
        if (!loadRetraceOptionsByThreadId(cmdOptions.tid))
        {
            DBG_LOG("Failed load header settings for thread id %d\n", cmdOptions.tid);
        }

    }

    if (mOptions.mRetraceTid == -1)
    {
        gRetracer.reportAndAbort("No thread ID specified in header - so you must specify it manually.");
    }

    if (cmdOptions.flushWork)
    {
        mOptions.mFlushWork = true;
    }

    if (cmdOptions.perfmon)
    {
        mOptions.mPerfmon = true;
    }

    if (cmdOptions.collectors || cmdOptions.collectors_streamline)
    {
        gRetracer.mCollectors = new Collection(Json::Value());
        std::vector<std::string> collectors = gRetracer.mCollectors->available();
        std::vector<std::string> filtered;
        gRetracer.mCollectors->setDebug(gRetracer.mOptions.mDebug);
        for (const std::string& s : collectors)
        {
            if ((s == "rusage" || s == "gpufreq" || s == "procfs" || s == "cpufreq" || s == "perf") && cmdOptions.collectors)
            {
                filtered.push_back(s);
            }
            else if (s == "streamline" && cmdOptions.collectors_streamline)
            {
                filtered.push_back(s);
                DBG_LOG("Streamline integration support enabled\n");
            }
        }
        if (!gRetracer.mCollectors->initialize(filtered))
        {
            fprintf(stderr, "Failed to initialize collectors\n");
        }
        DBG_LOG("libcollector instrumentation enabled through cmd line.\n");
    }

    if (cmdOptions.forceOffscreen)
    {
        mOptions.mForceOffscreen = true;
        // When running offscreen, force onscreen EGL to most compatible mode known: 5650 00
        mOptions.mOnscreenConfig = EglConfigInfo(5, 6, 5, 0, 0, 0, 0, 0);
        if (cmdOptions.singleFrameOffscreen)
        {
            // Draw offscreen using 1 big tile
            mOptions.mOnscrSampleH *= 10;
            mOptions.mOnscrSampleW *= 10;
            mOptions.mOnscrSampleNumX = 1;
            mOptions.mOnscrSampleNumY = 1;
        }
        mOptions.mOffscreenConfig.override(cmdOptions.eglConfig);
    }
    else
    {
        // RGBADS, are overriden either by forceOffscreen or overrideEGL
        mOptions.mOnscreenConfig.override(cmdOptions.eglConfig);
        if (cmdOptions.singleFrameOffscreen)
        {
            gRetracer.reportAndAbort("This option can only be used with offscreen rendering!");
        }
    }

    if (cmdOptions.forceSingleWindow)
    {
        mOptions.mForceSingleWindow = true;
    }

    if (cmdOptions.multiThread)
    {
        mOptions.mMultiThread = true;
    }

    if (mFile.getHeaderVersion() >= common::HEADER_VERSION_2
        && (cmdOptions.winW != mOptions.mWindowWidth || cmdOptions.winH != mOptions.mWindowHeight)
        && (cmdOptions.winW > 0 || cmdOptions.winH > 0))
    {
        DBG_LOG("Wrong window size specified, must be same as in trace header. This option is only useful for very old trace files!");
    }
    else if (cmdOptions.winW > 0 && cmdOptions.winH > 0)
    {
        DBG_LOG("Overriding default winsize %dx%d -> %dx%d\n",
                 mOptions.mWindowWidth, mOptions.mWindowHeight,
                 cmdOptions.winW, cmdOptions.winH);
        mOptions.mWindowWidth = cmdOptions.winW;
        mOptions.mWindowHeight = cmdOptions.winH;
    }
    if (mOptions.mWindowWidth <= 0 || mOptions.mWindowHeight <= 0)
    {
        // Either invalid header or cmd option
        DBG_LOG("Error: invalid window size (%dx%d) defined!\n", mOptions.mWindowWidth, mOptions.mWindowHeight);
        return false;
    }

    if (cmdOptions.oresW > 0 && cmdOptions.oresH > 0)
    {
        mOptions.mDoOverrideResolution = true;
        mOptions.mOverrideResW = cmdOptions.oresW;
        mOptions.mOverrideResH = cmdOptions.oresH;
        mOptions.mOverrideResRatioW = mOptions.mOverrideResW / (float) mOptions.mWindowWidth;
        mOptions.mOverrideResRatioH = mOptions.mOverrideResH / (float) mOptions.mWindowHeight;
        DBG_LOG("Override resolution enabled: %dx%d\n",
                 mOptions.mOverrideResW, mOptions.mOverrideResH);
        DBG_LOG("Override resolution ratio: %.2f x %.2f\n",
                 mOptions.mOverrideResRatioW, mOptions.mOverrideResRatioH);
    }

    if (cmdOptions.preload) {
        mOptions.mPreload = true;
    }

    if (cmdOptions.beginMeasureFrame >= cmdOptions.endMeasureFrame && cmdOptions.beginMeasureFrame >= 0)
    {
        gRetracer.reportAndAbort("Start frame must be lower than end frame. (End frame is never played.)");
    }
    else if (cmdOptions.beginMeasureFrame >= 0 && cmdOptions.endMeasureFrame >= 0)
    {
        mOptions.mBeginMeasureFrame = cmdOptions.beginMeasureFrame;
        mOptions.mEndMeasureFrame = cmdOptions.endMeasureFrame;
    }

    if (cmdOptions.snapshotCallSet != 0x0) {
        delete mOptions.mSnapshotCallSet;
        mOptions.mSnapshotCallSet = cmdOptions.snapshotCallSet;
    }

    if(cmdOptions.strictEGLMode) {
        mOptions.mStrictEGLMode = cmdOptions.strictEGLMode;
    }

    if(cmdOptions.strictColorMode) {
        mOptions.mStrictColorMode = cmdOptions.strictColorMode;
    }

    if (cmdOptions.skipCallSet != 0x0) {
        delete mOptions.mSkipCallSet;
        mOptions.mSkipCallSet = cmdOptions.skipCallSet;
    }

    if (cmdOptions.dmaSharedMemory) {
        mOptions.dmaSharedMemory = true;
    }

    return true;
}


static void CheckError(const char *callName, unsigned int callNo)
{
    if (callName)
    {
        if (strncmp(callName, "gl", 2) == 0)
        {
            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                const char * errorStr = NULL;
                if (error == GL_INVALID_ENUM)
                    errorStr = "GL_INVALID_ENUM";
                else if (error == GL_INVALID_VALUE)
                    errorStr = "GL_INVALID_VALUE";
                else if (error == GL_INVALID_OPERATION)
                    errorStr = "GL_INVALID_OPERATION";
                else if (error == GL_INVALID_FRAMEBUFFER_OPERATION)
                    errorStr = "GL_INVALID_FRAMEBUFFER_OPERATION";
                else if (error == GL_OUT_OF_MEMORY)
                    errorStr = "GL_OUT_OF_MEMORY";
                else
                    errorStr = "Unknown GL error";
                DBG_LOG("GL_ERROR [%d %s] %s\n", callNo, callName, errorStr);
            }
        }
        else if (strncmp(callName, "egl", 3) == 0)
        {
            EGLenum error = eglGetError();
            if (error != EGL_SUCCESS)
            {
                DBG_LOG("EGL_ERROR [%d %s] %d\n", callNo, callName, error);
            }
        }
    }
}

bool Retracer::RetraceForward(unsigned int frameNum, unsigned int drawNum, bool dumpFrameBuffer)
{
    UnCompressedChunk *callChunk;

    while (frameNum || drawNum)
    {
        void* fptr;
        char* src;
        if (!mFile.GetNextCall(fptr, mCurCall, src, callChunk))
        {
            wakeupAllWorkThreads();
            waitWorkThreadPoolIdle();

            GLWS::instance().Cleanup();

            CloseTraceFile();
            return false;
        }

        if (!mOptions.mMultiThread && getCurTid() != mOptions.mRetraceTid)
        {
            mCurCallNo++;
            continue;
        }

        const char *callName = mFile.ExIdToName(mCurCall.funcId);
        const bool isSwapBuffers = (mCurCall.funcId == mExIdEglSwapBuffers || mCurCall.funcId == mExIdEglSwapBuffersWithDamage);

        if (fptr)
        {
            if (!mOptions.mMultiThread)
            {
                (*(RetraceFunc)fptr)(src);
                CheckError(callName, mCurCallNo);
            }
            else
            {
                Work* work = new Work(mCurCall.tid, mDispatchFrameNo, mCurCallNo, fptr, src, callName, callChunk);
                DispatchWork(work);
                WorkThread* wThread = findWorkThread(mCurCall.tid);
                wThread->waitIdle();
            }
            if (isSwapBuffers && mCurCall.tid==mOptions.mRetraceTid)
            {
                mDispatchFrameNo++;
            }
        }
        else
        {
            DBG_LOG("    Unsupported function : callNo %d\n", mCurCallNo);
        }

        if (isSwapBuffers && mCurCall.tid==mOptions.mRetraceTid)
        {
            if (frameNum) frameNum--;
        }

        if ((dumpFrameBuffer && frameNum == 0 && drawNum == 0) || (drawNum == 0 && frameNum == 0))
        {
            if (!mOptions.mMultiThread)
            {
                StepShot(mCurCallNo, mDispatchFrameNo);
            }
            else
            {
                StepWork* work = new StepWork(mCurCall.tid, mDispatchFrameNo, mCurCallNo, NULL, NULL, callName, NULL);
                DispatchWork(work);
            }
        }

        mCurCallNo++;
    }

    return true;
}

std::vector<Texture> Retracer::getTexturesToDump()
{
    static const char* todump = getenv("RETRACE_DUMP_TEXTURES");

    if(!todump)
    {
        return std::vector<Texture>();
    }

    // Format:
    // "texid texid ..."
    std::string str(todump);
    std::istringstream textureNamesSS(str);

    std::vector<Texture> ret;
    GLint textureName = 0;
    Context& context = gRetracer.getCurrentContext();
    while (textureNamesSS >> textureName)
    {
        Texture t;
        t.handle = context.getTextureMap().RValue(textureName);
        ret.push_back(t);
    }
    return ret;
}

void Retracer::dumpUniformBuffers(unsigned int callNo)
{
    static const char* todump = getenv("RETRACE_DUMP_BUFFERS");

    if(!todump)
    {
        return;
    }

    State s({}, {GL_UNIFORM_BUFFER_BINDING});

    // Format = "bufId bufId ..."
    std::string str(todump);
    std::istringstream ubNamesSS(str);
    GLint ubName = 0;
    Context& context = gRetracer.getCurrentContext();
    while (ubNamesSS >> ubName)
    {
        GLint ubNameNew = context.getBufferMap().RValue(ubName);
        if (glIsBuffer(ubNameNew) == GL_FALSE)
        {
            DBG_LOG("%d is not a buffer.\n", ubNameNew);
            continue;
        }

        glBindBuffer(GL_UNIFORM_BUFFER, ubNameNew);
        GLint size = 0;
        glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_SIZE, &size);
        GLint is_mapped = 0;
        glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_MAPPED, &is_mapped);
        if(is_mapped == GL_TRUE)
        {
            glUnmapBuffer(GL_UNIFORM_BUFFER);
        }
        char* bufferdata = (char*)glMapBufferRange(GL_UNIFORM_BUFFER,
                                                   0,
                                                   size,
                                                   GL_MAP_READ_BIT);
        if (!bufferdata)
        {
            DBG_LOG("Couldn't map buffer %d\n", ubName);
            continue;
        }

        DBG_LOG("Bound buffer %d, size %d\n", ubName, size);
        //
        // Grab data
        std::stringstream ss;
        ss << mOptions.mSnapshotPrefix << std::setw(10) << std::setfill('0') << callNo << "_b" << ubName << ".bin";

        FILE* fd = fopen(ss.str().c_str(), "w");
        if (!fd)
        {
            DBG_LOG("Unable to open %s\n", ss.str().c_str());
            continue;
        }
        fwrite(bufferdata, 1, size, fd);
        fclose(fd);
        DBG_LOG("Wrote %s\n", ss.str().c_str());
    }
}

void Retracer::StepShot(unsigned int callNo, unsigned int frameNo, const char *filename)
{
    DBG_LOG("[%d] [Frame/Draw/Call %d/%d/%d] %s.\n", getCurTid(), frameNo, GetCurDrawId(), callNo, GetCurCallName());

    if (eglGetCurrentContext() != EGL_NO_CONTEXT)
    {
        GLint maxAttachments = getMaxColorAttachments();
        GLint maxDrawBuffers = getMaxDrawBuffers();
        for(int i=0; i<maxDrawBuffers; i++)
        {
            GLint colorAttachment = getColorAttachment(i);
            if(colorAttachment != GL_NONE)
            {
                image::Image *src = getDrawBufferImage(colorAttachment);
                if (src == NULL)
                {
                    DBG_LOG("Failed to dump bound framebuffer\n");
                    break;
                }

                std::string filenameToBeUsed;
                int attachmentIndex = colorAttachment - GL_COLOR_ATTACHMENT0;
                bool validAttachmentIndex = attachmentIndex >= 0 && attachmentIndex < maxAttachments;
                if (!validAttachmentIndex)
                {
                    attachmentIndex = 0;
                }

                std::stringstream ss;
                ss << "framebuffer" << "_c" << attachmentIndex << ".png";
                filenameToBeUsed = ss.str();

                if (src->writePNG(filenameToBeUsed.c_str()))
                {
                    DBG_LOG("Dump bound framebuffer to %s\n", filenameToBeUsed.c_str());
                }
                else
                {
                    DBG_LOG("Failed to dump bound framebuffer to %s\n", filenameToBeUsed.c_str());
                }
                delete src;
            }
        }
    }
    else
    {
        DBG_LOG("EGL context has not been created\n");
    }
}

void Retracer::Flush()
{
    _glFinish();
}

void Retracer::TakeSnapshot(unsigned int callNo, unsigned int frameNo, const char *filename)
{
    // Only take snapshots inside the measurement range
    const bool inRange = mOptions.mBeginMeasureFrame <= frameNo && frameNo <= mOptions.mEndMeasureFrame;
    if (mOptions.mUploadSnapshots && !inRange)
    {
        return;
    }
    // Add state log dumps, if these are enabled, at the same time
    if (mOptions.mStateLogging)
    {
        mStateLogger.logState(getCurTid());
    }

    GLint maxAttachments = getMaxColorAttachments();
    GLint maxDrawBuffers = getMaxDrawBuffers();
    bool colorAttach = false;
    for (int i=0; i<maxDrawBuffers; i++)
    {
        GLint colorAttachment = getColorAttachment(i);
        if(colorAttachment != GL_NONE)
        {
            colorAttach = true;
            int readFboId = 0, drawFboId = 0;
            _glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);
            _glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
            const unsigned int ON_SCREEN_FBO = 1;
#else
            const unsigned int ON_SCREEN_FBO = 0;
#endif
            if (gRetracer.mOptions.mForceOffscreen) {
                _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ON_SCREEN_FBO);
                _glBindFramebuffer(GL_READ_FRAMEBUFFER, drawFboId);
            }
            else {
                _glBindFramebuffer(GL_READ_FRAMEBUFFER, drawFboId);
            }
            image::Image *src = getDrawBufferImage(colorAttachment);
            _glBindFramebuffer(GL_READ_FRAMEBUFFER, readFboId);
            _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFboId);
            if (src == NULL)
            {
                DBG_LOG("Failed to take snapshot for call no: %d\n", callNo);
                return;
            }

            std::string filenameToBeUsed;
            if (filename)
            {
                filenameToBeUsed = filename;
            }
            else // no incoming filename, we must generate one
            {
                int attachmentIndex = colorAttachment - GL_COLOR_ATTACHMENT0;
                bool validAttachmentIndex = attachmentIndex >= 0 && attachmentIndex < maxAttachments;
                if (!validAttachmentIndex)
                {
                    attachmentIndex = 0;
                }

                std::stringstream ss;
                if (mOptions.mSnapshotFrameNames)
                {
                    ss << mOptions.mSnapshotPrefix << std::setw(4) << std::setfill('0') << frameNo << ".png";
                }
                else
                {
                    ss << mOptions.mSnapshotPrefix << std::setw(10) << std::setfill('0') << callNo << "_c" << attachmentIndex << ".png";
                }
                filenameToBeUsed = ss.str();
            }

            if (src->writePNG(filenameToBeUsed.c_str()))
            {
                DBG_LOG("Snapshot (frame %d, call %d) : %s\n", frameNo, callNo, filenameToBeUsed.c_str());

                // Register the snapshot to be uploaded
                if (mOptions.mUploadSnapshots)
                {
                    mSnapshotPaths.push_back(filenameToBeUsed);
                }
            }
            else
            {
                DBG_LOG("Failed to write snapshot : %s\n", filenameToBeUsed.c_str());
            }

            delete src;
        }
    }
    if (!colorAttach)   // no color attachment, there might be a depth attachment
    {
        DBG_LOG("no color attachment, there might be a depth attachment\n");
        int readFboId = 0, drawFboId = 0;
        _glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);
        _glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        const unsigned int ON_SCREEN_FBO = 1;
#else
        const unsigned int ON_SCREEN_FBO = 0;
#endif
        if (gRetracer.mOptions.mForceOffscreen) {
            _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ON_SCREEN_FBO);
            gRetracer.mpOffscrMgr->BindOffscreenReadFBO();
        }
        else {
            _glBindFramebuffer(GL_READ_FRAMEBUFFER, drawFboId);
        }
        image::Image *src = getDrawBufferImage(GL_DEPTH_ATTACHMENT);
        _glBindFramebuffer(GL_READ_FRAMEBUFFER, readFboId);
        _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFboId);
        if (src == NULL)
        {
            DBG_LOG("Failed to take snapshot for call no: %d\n", callNo);
            return;
        }

        std::string filenameToBeUsed;
        if (filename)
        {
            filenameToBeUsed = filename;
        }
        else // no incoming filename, we must generate one
        {
            std::stringstream ss;
            if (mOptions.mSnapshotFrameNames)
            {
                ss << mOptions.mSnapshotPrefix << std::setw(4) << std::setfill('0') << frameNo << ".png";
            }
            else
            {
                ss << mOptions.mSnapshotPrefix << std::setw(10) << std::setfill('0') << callNo << "_depth.png";
            }
            filenameToBeUsed = ss.str();
        }

        if (src->writePNG(filenameToBeUsed.c_str()))
        {
            DBG_LOG("Snapshot (frame %d, call %d) : %s\n", frameNo, callNo, filenameToBeUsed.c_str());

            // Register the snapshot to be uploaded
            if (mOptions.mUploadSnapshots)
            {
                mSnapshotPaths.push_back(filenameToBeUsed);
            }
        }
        else
        {
            DBG_LOG("Failed to write snapshot : %s\n", filenameToBeUsed.c_str());
        }

        delete src;
    }

    std::vector<Texture> textures = getTexturesToDump();
    GLfloat vertices[8] = { 0.0f, 1.0f,
                            0.0f, 0.0f,
                            1.0f, 1.0f,
                            1.0f, 0.0f };
    for (std::vector<Texture>::iterator it = textures.begin(); it != textures.end(); ++it)
    {
        dumpTexture(*it, callNo, &vertices[0]);
    }

    dumpUniformBuffers(callNo);
}

#if 0
std::string GetShader(const char* filepath)
{
    ifstream ifs;
    ifs.open(filepath);
    if (!ifs.is_open())
        DBG_LOG("Failed to open shader source: %s\n", filepath);
    else
        DBG_LOG("Open shader source: %s\n", filepath);

    std::string ret;
    std::string line;

    while (getline(ifs, line)) {
        DBG_LOG("%s\n", line.c_str());
        ret += line;
        ret += "\n";
    }

    return ret;
}

void DumpUniforms()
{
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    if (!program)
        return;

    GLint uniCount;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniCount);


    char uniName[256];
    const int UNI_VALUE_BYTE_LEN = 10*1024;
    char uniValue[UNI_VALUE_BYTE_LEN];

    for (int uniLoc = 0; uniLoc < uniCount; ++uniLoc)
    {
        GLint   uniSize;
        GLenum  uniType;
        glGetActiveUniform(program, uniLoc, 256, NULL, &uniSize, &uniType, uniName);

        GLint location;
        location = glGetUniformLocation(program, uniName);

        DBG_LOG("Active idx: #%d, Location: %d\n", uniLoc, location);
        if (uniType == GL_FLOAT) {
            if (uniSize * sizeof(float) > UNI_VALUE_BYTE_LEN)
                DBG_LOG("Error: UNI_VALUE_BYTE_LEN should be increased!!\n");

            glGetUniformfv(program, location, ((GLfloat*)uniValue));
            DBG_LOG("%s float=%f\n", uniName, ((GLfloat*)uniValue)[0]);
        } else if (uniType == GL_FLOAT_VEC2) {
            if (uniSize * 2*sizeof(float) > UNI_VALUE_BYTE_LEN)
                DBG_LOG("Error: UNI_VALUE_BYTE_LEN should be increased!!\n");

            glGetUniformfv(program, location, ((GLfloat*)uniValue));
            DBG_LOG("%s float2=%f\n", uniName, ((GLfloat*)uniValue)[0]);
        } else if (uniType == GL_FLOAT_VEC3) {
            if (uniSize * 3*sizeof(float) > UNI_VALUE_BYTE_LEN)
                DBG_LOG("Error: UNI_VALUE_BYTE_LEN should be increased!!\n");

            glGetUniformfv(program, location, ((GLfloat*)uniValue));
            DBG_LOG("%s float3=%f\n", uniName, ((GLfloat*)uniValue)[0]);
        } else if (uniType == GL_FLOAT_VEC4) {
            if (uniSize * 4*sizeof(float) > UNI_VALUE_BYTE_LEN)
                DBG_LOG("Error: UNI_VALUE_BYTE_LEN should be increased!!\n");

            glGetUniformfv(program, location, ((GLfloat*)uniValue));
            DBG_LOG("%s float4=%f\n", uniName, ((GLfloat*)uniValue)[0]);
        } else if (uniType == GL_INT || uniType == GL_BOOL || uniType == GL_SAMPLER_2D || uniType == GL_SAMPLER_CUBE) {
            if (uniSize * sizeof(int) > UNI_VALUE_BYTE_LEN)
                DBG_LOG("Error: UNI_VALUE_BYTE_LEN should be increased!!\n");

            glGetUniformiv(program, location, ((GLint*)uniValue));
            DBG_LOG("%s int=%d\n", uniName, ((GLint*)uniValue)[0]);
        } else if (uniType == GL_INT_VEC2 || uniType == GL_BOOL_VEC2) {
            if (uniSize * 2*sizeof(int) > UNI_VALUE_BYTE_LEN)
                DBG_LOG("Error: UNI_VALUE_BYTE_LEN should be increased!!\n");

            glGetUniformiv(program, location, ((GLint*)uniValue));
            DBG_LOG("%s int2=%d\n", uniName, ((GLint*)uniValue)[0]);
        } else if (uniType == GL_INT_VEC3 || uniType == GL_BOOL_VEC3) {
            if (uniSize * 3*sizeof(int) > UNI_VALUE_BYTE_LEN)
                DBG_LOG("Error: UNI_VALUE_BYTE_LEN should be increased!!\n");

            glGetUniformiv(program, location, ((GLint*)uniValue));
            DBG_LOG("%s int3=%d\n", uniName, ((GLint*)uniValue)[0]);
        } else if (uniType == GL_INT_VEC4 || uniType == GL_BOOL_VEC4) {
            if (uniSize * 4*sizeof(int) > UNI_VALUE_BYTE_LEN)
                DBG_LOG("Error: UNI_VALUE_BYTE_LEN should be increased!!\n");

            glGetUniformiv(program, location, ((GLint*)uniValue));
            DBG_LOG("%s int4=%d\n", uniName, ((GLint*)uniValue)[0]);
        } else if (uniType == GL_FLOAT_MAT2) {
            if (uniSize * 4*sizeof(float) > UNI_VALUE_BYTE_LEN)
                DBG_LOG("Error: UNI_VALUE_BYTE_LEN should be increased!!\n");

            glGetUniformfv(program, location, ((GLfloat*)uniValue));
            DBG_LOG("%s float2x2=%f\n", uniName, ((GLfloat*)uniValue)[0]);
        } else if (uniType == GL_FLOAT_MAT3) {
            if (uniSize * 9*sizeof(float) > UNI_VALUE_BYTE_LEN)
                DBG_LOG("Error: UNI_VALUE_BYTE_LEN should be increased!!\n");

            glGetUniformfv(program, location, ((GLfloat*)uniValue));
            DBG_LOG("%s float3x3=%f\n", uniName, ((GLfloat*)uniValue)[0]);
        } else if (uniType == GL_FLOAT_MAT4) {
            if (uniSize * 16*sizeof(float) > UNI_VALUE_BYTE_LEN)
                DBG_LOG("Error: UNI_VALUE_BYTE_LEN should be increased!!\n");

            glGetUniformfv(program, location, ((GLfloat*)uniValue));
            DBG_LOG("%s float4x4=%f\n", uniName, ((GLfloat*)uniValue)[0]);
        } else {
            DBG_LOG("Unknown uniform type: %d\n", uniType);
        }

        if (uniType == GL_SAMPLER_2D)
        {
            GLint oldActTex;
            _glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&oldActTex);

            GLint actTex = ((GLint*)uniValue)[0];
            _glActiveTexture(GL_TEXTURE0+actTex);
            GLint bindTex;
            _glGetIntegerv(GL_TEXTURE_BINDING_2D, &bindTex);
            DBG_LOG("    texture name: %d\n", bindTex);

            _glActiveTexture(oldActTex);
        }
    }

    GLint shdLoc;
    shdLoc = glGetUniformLocation(program, "unity_World2Shadow[0]");
    DBG_LOG("unity_World2Shadow[0] = %d\n", shdLoc);
    shdLoc = glGetUniformLocation(program, "unity_World2Shadow[1]");
    DBG_LOG("unity_World2Shadow[1] = %d\n", shdLoc);
    shdLoc = glGetUniformLocation(program, "unity_World2Shadow[2]");
    DBG_LOG("unity_World2Shadow[2] = %d\n", shdLoc);
    shdLoc = glGetUniformLocation(program, "unity_World2Shadow[3]");
    DBG_LOG("unity_World2Shadow[3] = %d\n", shdLoc);
}
#endif


#ifdef ANDROID
string getProcessNameByPid(const int pid)
{
    string pname;
    ifstream f("/proc/" + _to_string(pid) + "/cmdline");
    if (f.is_open())
    {
        f >> pname;
        f.close();
    }
    return pname;
}

#ifdef PLATFORM_64BIT
    #define SYSTEM_VENDOR_LIB_PREFIX "/system/vendor/lib64/egl/"
    #define SYSTEM_LIB_PREFIX "/system/lib64/egl/"
#else
    #define SYSTEM_VENDOR_LIB_PREFIX "/system/vendor/lib/egl/"
    #define SYSTEM_LIB_PREFIX "/system/lib/egl/"
#endif

// Configuration files
const char* applist_cfg_search_paths[] = {
    SYSTEM_VENDOR_LIB_PREFIX "appList.cfg",
    SYSTEM_LIB_PREFIX "appList.cfg",
    NULL,
};

const char* findFirst(const char** paths)
{
    const char* i;
    const char* last = "NONE";
    while ((i = *paths++))
    {
        if (access(i, R_OK) == 0)
        {
             return i;
        }
        last = i;
    }
    return last; // for informative error message and avoid crashing
}
#endif

void forceRenderMosaicToScreen()
{
    if (gRetracer.mMosaicNeedToBeFlushed)
    {
        if (gRetracer.mpOffscrMgr->last_tid != -1) {
            gRetracer.mCurCall.tid = gRetracer.mpOffscrMgr->last_tid;

            int last_non_zero_draw = gRetracer.mpOffscrMgr->last_non_zero_draw;
            int last_non_zero_ctx = gRetracer.mpOffscrMgr->last_non_zero_ctx;

            if (!gRetracer.mState.GetDrawable(last_non_zero_draw)) {
                int win = gRetracer.mState.GetWin(last_non_zero_draw);
                int parameter[8];
                parameter[0] = 0;                   // dpy, useless in retrace_eglCreateWindowSurface
                parameter[1] = 0;                   // config, useless in retrace_eglCreateWindowSurface
                parameter[2] = win;                 // native_window
                parameter[3] = sizeof(int) * 3;     // byte size of attrib_list
                parameter[4] = EGL_RENDER_BUFFER;
                parameter[5] = EGL_BACK_BUFFER;
                parameter[6] = EGL_NONE;
                parameter[7] = last_non_zero_draw;  // ret
                retrace_eglCreateWindowSurface(reinterpret_cast<char *>(parameter));
            }

            int parameter[5];
            parameter[0] = 0;                   // dpy, useless in retrace_eglMakeCurrent
            parameter[1] = last_non_zero_draw;  // draw
            parameter[2] = last_non_zero_draw;  // read, useless in retrace_eglMakeCurrent
            parameter[3] = last_non_zero_ctx;   // ctx
            parameter[4] = 1;                   // ret, useless in retrace_eglMakeCurrent
            retrace_eglMakeCurrent(reinterpret_cast<char *>(parameter));

            gRetracer.mpOffscrMgr->MosaicToScreenIfNeeded(true);
            retracer::Drawable* pDrawable = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getDrawable();
            if (pDrawable != NULL) {
                pDrawable->swapBuffers();
            }
            else {
                DBG_LOG("pDrawable == NULL. The mosaic picture of last several frames can't be rendered to screen. This might be a bug.\n");
            }
        }
    }
}

static void report_cpu_mask()
{
    cpu_set_t mask;
    std::string descr;
    int retval = sched_getaffinity(0, sizeof(mask), &mask);
    if (retval != 0)
    {
        DBG_LOG("Failed to get CPU mask: %s\n", strerror(errno));
    }
    for (unsigned i = 0; i < sizeof(mask) / CPU_ALLOC_SIZE(1); i++)
    {
        descr += CPU_ISSET(i, &mask) ? "1" : "0";
    }
    while (descr.back() == '0') descr.pop_back(); // on Android, string will be very long otherwise
    DBG_LOG("Current CPU mask: %s\n", descr.c_str());
}

static void set_cpu_mask(const std::string& descr)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    for (unsigned i = 0; i < descr.size(); i++)
    {
        if (descr.at(i) == '1')
        {
            CPU_SET(i, &mask);
        }
        else if (descr.at(i) != '0')
        {
            DBG_LOG("Invalid CPU mask: %s!\n", descr.c_str());
            return;
        }
    }
    int retval = sched_setaffinity(0, sizeof(mask), &mask);
    if (retval != 0)
    {
        DBG_LOG("Failed to set CPU mask: %s\n", strerror(errno));
    }
}

void Retracer::perfMonInit()
{
    perfmon_init();
    delayedPerfmonInit = false;
}

bool Retracer::Retrace()
{
    void* fptr;
    char* src;
    UnCompressedChunk *callChunk;

    if (!mOptions.mCpuMask.empty()) set_cpu_mask(mOptions.mCpuMask);
    report_cpu_mask();

    // prepare for the last glFinish
    const char *glFinish_name = "glFinish";
    int glFinish_id = mFile.NameToExId(glFinish_name);
    void *glFinish_fptr = gApiInfo.NameToFptr(glFinish_name);
    BCall glFinish_call;
    glFinish_call.funcId = glFinish_id;
    glFinish_call.reserved = 0;
    char *glFinish_src = (char *)&glFinish_call;

    // Only do the preloading setting at the beginning.
    if (mDispatchFrameNo == 0 && mOptions.mPreload)
    {
        mFile.SetPreloadRange(mOptions.mBeginMeasureFrame, mOptions.mEndMeasureFrame, mOptions.mRetraceTid);
    }

    if (!mOptions.mPreload && mOptions.mBeginMeasureFrame == 0 && mCurFrameNo == 0)
    {
        StartMeasuring(); // special case, otherwise triggered by eglSwapBuffers()
        if (mOptions.mPerfmon) delayedPerfmonInit = true;
    }

    for (;;mCurCallNo++)
    {
        if (!mFile.GetNextCall(fptr, mCurCall, src, callChunk) || mFinish)
        {
            wakeupAllWorkThreads();
            waitWorkThreadPoolIdle();
            if (mOptions.mForceOffscreen) {
                char empty_src[1] = "";
                const char *forceRenderMosaicToScreenName = "forceRenderMosaicToScreen";
                DispatchWork(mCurCall.tid, -1, -1, (void*)forceRenderMosaicToScreen, empty_src, forceRenderMosaicToScreenName, 0);
            }

            glFinish_call.tid = mCurCall.tid;
            DispatchWork(mCurCall.tid, mDispatchFrameNo, mCurCallNo, glFinish_fptr, glFinish_src, glFinish_name, 0);
            if (!mOutOfMemory)
            {
                saveResult();
            }
            GLWS::instance().Cleanup();
            CloseTraceFile();
#if ANDROID
            if (!mOptions.mForceSingleWindow)
            {
                // Figure out if we want to intercept paretracer itself.
                const char* appListPath = findFirst(applist_cfg_search_paths);
                ifstream appListIfs(appListPath);
                if (!appListIfs.is_open())  // no appList.cfg, user doesn't want to trace paretrace
                {
                    return false;
                }
                pid_t pid = getpid();
                string itAppName;
                string pname = getProcessNameByPid(getpid());
                while (appListIfs >> itAppName)
                {
                    if (!strcmp(itAppName.c_str(), pname.c_str()))    // user wants to trace paretrace
                    {
                        kill(pid, SIGTERM);
                        break;
                    }
                }
            }
#endif
            return false;
        }

        if (mOptions.mPreload && mOptions.mBeginMeasureFrame==0 && mTimerBeginTime==0)
        {
            StartMeasuring(); // special case for preload from 0. otherwise triggered by eglSwapBuffers
            if (mOptions.mPerfmon) delayedPerfmonInit = true;
        }

        if (!mOptions.mMultiThread && mCurCall.tid != mOptions.mRetraceTid)
        {
            continue;
        }

        const bool doTakeSnapshot = mOptions.mSnapshotCallSet &&
            (mOptions.mSnapshotCallSet->contains(mCurCallNo, mFile.ExIdToName(mCurCall.funcId)));

        const bool doFrameTakeSnapshot = mOptions.mSnapshotCallSet &&
            (mOptions.mSnapshotCallSet->contains(mDispatchFrameNo, mFile.ExIdToName(mCurCall.funcId)));

        const bool isSwapBuffers = (mCurCall.funcId == mExIdEglSwapBuffers || mCurCall.funcId == mExIdEglSwapBuffersWithDamage);

        if (doFrameTakeSnapshot && isSwapBuffers)
        {
            if (!mOptions.mMultiThread)
            {
                // Single thread mode
                this->TakeSnapshot(mCurCallNo-1, mDispatchFrameNo);
            }
            else
            {
                SnapshotWork* work = new SnapshotWork(mCurCall.tid, mDispatchFrameNo, mCurCallNo - 1, NULL, NULL, "Snapshot", NULL);
                DispatchWork(work);
            }
        }

        const char *funcName = mFile.ExIdToName(mCurCall.funcId);

        bool doSkip = mOptions.mSkipCallSet && (mOptions.mSkipCallSet->contains(mCurCallNo, funcName));

        if (fptr)
        {
            bool discarded = false;
            // discard work if skipwork enabled and outside measured frame range
            if (mOptions.mSkipWork >= 0 && (mDispatchFrameNo + mOptions.mSkipWork < mOptions.mBeginMeasureFrame || mDispatchFrameNo >= mOptions.mEndMeasureFrame))
            {
                if (strncmp(funcName, "eglSwapBuffers", strlen("eglSwapBuffers")) == 0 || strcmp(funcName, "glReadPixels") == 0 || strcmp(funcName, "glFlush") == 0
                    || strcmp(funcName, "glFinish") == 0 || strcmp(funcName, "glBindFramebuffer") == 0)
                {
                    if (!mOptions.mMultiThread)
                    {
                        DiscardFramebuffers();
                    }
                    else
                    {
                        DiscardFramebuffersWork* work = new DiscardFramebuffersWork(mCurCall.tid, mDispatchFrameNo, mCurCallNo, NULL, NULL, "DiscardFramebuffers", NULL);
                        DispatchWork(work);
                    }
                    discarded = true;
                }
                else if (strcmp(funcName, "glDispatchCompute") == 0 || strcmp(funcName, "glDispatchComputeIndirect") == 0)
                {
                    doSkip = true;
                }
            }

            if (mOptions.mDebug > 1 || (mOptions.mDebug && (doSkip || discarded)))
            {
                DBG_LOG("    %s : c%d f%d%s%s\n", funcName, mCurCallNo, mDispatchFrameNo, doSkip ? " (skipped)" : "", discarded ? " (discarded)" : "");
            }

            if (!doSkip)
            {
                if (isSwapBuffers && mOptions.mFinishBeforeSwap)
                {
                    if (!mOptions.mMultiThread)
                    {
                        _glFinish();
                    }
                    else
                    {
                        FlushWork* work = new FlushWork(mCurCall.tid, mDispatchFrameNo, mCurCallNo - 1, NULL, NULL, "Flush", NULL);
                        DispatchWork(work);
                    }
                }
                DispatchWork(mCurCall.tid, mDispatchFrameNo, mCurCallNo, fptr, src, funcName, callChunk);
                if (isSwapBuffers && mCurCall.tid == mOptions.mRetraceTid)
                {
                    mDispatchFrameNo++;
                    if (mOptions.mPerfmon) perfmon_frame();
                }
            }
            else
            {
                continue;
            }
        }
        else if (mOptions.mDebug)
        {
            DBG_LOG("    Unsupported function : %s, call no: %d\n", funcName, mCurCallNo);
        }

        if (isSwapBuffers)
        {
            if (mOptions.mPerfStart == (int)mDispatchFrameNo + 1) // before perf frame
            {
                if (!mOptions.mMultiThread)
                {
                    // Single thread mode
                    PerfStart();
                }
                else
                {
                    PerfWork* work = new PerfWork(mCurCall.tid, mDispatchFrameNo, mCurCallNo, NULL, NULL, "PerfStart", NULL);
                    DispatchWork(work);
                }
            }
            else if (mOptions.mPerfStop == (int)mDispatchFrameNo) // last frame
            {
                if (!mOptions.mMultiThread)
                {
                    // Single thread mode
                    PerfEnd();
                }
                else
                {
                    PerfWork* work = new PerfWork(mCurCall.tid, mDispatchFrameNo, mCurCallNo, NULL, NULL, "PerfEnd", NULL);
                    DispatchWork(work);
                }
            }
        }
        else if (doTakeSnapshot)
        {
            if (!mOptions.mMultiThread)
            {
                // Single thread mode
                this->TakeSnapshot(mCurCallNo, mDispatchFrameNo);
            }
            else
            {
                SnapshotWork* work = new SnapshotWork(mCurCall.tid, mDispatchFrameNo, mCurCallNo, NULL, NULL, "Snapshot", NULL);
                DispatchWork(work);
            }
        }

        // End conditions
        if (mDispatchFrameNo >= mOptions.mEndMeasureFrame || mOutOfMemory || mFailedToLinkShaderProgram)
        {
            mFinish = true;
        }
    }
}

void Retracer::CheckGlError() {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) {
        return;
    }

    DBG_LOG("[%d] %s  ERR: %d \n", GetCurCallId(), GetCurCallName(), error);
}

void Retracer::OnFrameComplete()
{
    if (getCurTid() == mOptions.mRetraceTid)
    {
        // Per frame measurement
        if (mOptions.mMeasureSwapTime && mEndFrameTime)
        {
            getDuration(mEndFrameTime, &mFinishSwapTime);
        }
    }
}

void Retracer::StartMeasuring()
{
    if (mCollectors)
    {
        mCollectors->start();
    }
    DBG_LOG("================== Start timer (Frame: %u) ==================\n", mCurFrameNo);
    mTimerBeginTime = os::getTime();
    mEndFrameTime = mTimerBeginTime;
}

void Retracer::OnNewFrame()
{
    if (getCurTid() == mOptions.mRetraceTid)
    {
        // swap (or flush for offscreen) called between OnFrameComplete() and OnNewFrame()
        mCurFrameNo++;

        // End conditions
        if (mCurFrameNo >= mOptions.mEndMeasureFrame || mOutOfMemory || mFailedToLinkShaderProgram)
        {
            mFinish = true;
        }

        if (mCurFrameNo == mOptions.mBeginMeasureFrame)
        {
            if (mOptions.mFlushWork)
            {
                // First try to flush all the work we can
                _glFlush(); // force all GPU work to complete before this point
                const auto programs = gRetracer.getCurrentContext().getProgramMap().GetCopy();
                for (const auto program : programs) // force all compiler work to complete before this point
                {
                    GLint size = 0;
                    _glGetProgramiv(program.second, GL_PROGRAM_BINARY_LENGTH, &size);
                }
                sync(); // force all pending output to disk before this point
            }
            StartMeasuring();
            if (mOptions.mPerfmon) perfmon_init();
        }
        // Per frame measurement
        if (mCollectors && mCurFrameNo > mOptions.mBeginMeasureFrame && mCurFrameNo <= mOptions.mEndMeasureFrame)
        {
            mCollectors->collect();
        }
    }
}

void Retracer::PerfStart()
{
    pid_t parent = getpid();
    child = fork();
    if (child == -1)
    {
        DBG_LOG("Failed to fork: %s\n", strerror(errno));
    }
    else if (child == 0)
    {
        std::string freqopt = "--freq=" + _to_string(mOptions.mPerfFreq);
        std::string mypid = "--pid=" + _to_string(parent);
        std::string myfilename = mOptions.mPerfOut;
        std::string myfilearg = "--output=" + myfilename;
        const char* args[7] = { mOptions.mPerfPath.c_str(), "record", "-g", freqopt.c_str(), mypid.c_str(), myfilearg.c_str(), nullptr };
        DBG_LOG("Perf tracing %ld from process %ld with freq %ld and output in %s\n", (long)parent, (long)getpid(), (long)mOptions.mPerfFreq, myfilename.c_str());
        DBG_LOG("Perf args: %s %s %s %s %s %s\n", args[0], args[1], args[2], args[3], args[4], args[5]);
        if (execv(args[0], (char* const*)args) == -1)
        {
            DBG_LOG("Failed execv() for perf: %s\n", strerror(errno));
        }
    }
    else
    {
        sleep(1); // nasty hack needed because otherwise we could sometimes finish before the child process could get started
    }
}

void Retracer::PerfEnd()
{
    DBG_LOG("Killing instrumented process %ld\n", (long)child);
    if (kill(child, SIGINT) == -1)
    {
        DBG_LOG("Failed to send SIGINT to perf process: %s\n", strerror(errno));
    }
    usleep(200); // give it some time to exit, before proceeding
}

void Retracer::DiscardFramebuffers()
{
    GLint v = 0;
    _glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &v);
    if (v != 0)
    {
        static std::vector<GLenum> attachments = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
                                                   GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
        _glDiscardFramebufferEXT(GL_FRAMEBUFFER, attachments.size(), attachments.data());
    }
    else
    {
        static std::vector<GLenum> attachments = { GL_COLOR_EXT, GL_DEPTH_EXT, GL_STENCIL_EXT };
        _glDiscardFramebufferEXT(GL_FRAMEBUFFER, attachments.size(), attachments.data());
    }
}

void Retracer::reportAndAbort(const char *format, ...)
{
    char buf[256];
    va_list ap;
    memset(buf, 0, sizeof(buf));
    if ( mOptions.mMultiThread && (!workThreadPool[getCurTid()] || !GetCurWork(getCurTid())) )
    {
        sprintf(buf, "[c%u,f%u] ", 0, 0);
    }
    else
    {
        sprintf(buf, "[c%u,f%u] ", GetCurCallId(), GetCurFrameId());
    }
    va_start(ap, format);
    const unsigned len = strlen(buf);
    vsnprintf(buf + len, sizeof(buf) - len - 1, format, ap);
    va_end(ap);
    TraceExecutor::writeError(TRACE_ERROR_GENERIC, buf);
#ifdef __APPLE__
     throw PA_EXCEPTION(buf);
#elif ANDROID
    sleep(1); // give log call a chance before world dies below...
    ::exit(0); // too many failure calls will cause Android to hate us and blacklist our app until we uninstall it
#else
    ::abort();
#endif
}

void Retracer::saveResult()
{
    int64_t endTime;
    float duration = getDuration(mTimerBeginTime, &endTime);
    unsigned int numOfFrames = mCurFrameNo - mOptions.mBeginMeasureFrame;
    Json::Value result;

    if(mTimerBeginTime != 0) {
        const float fps = ((double)numOfFrames) / duration;
        DBG_LOG("================== End timer (Frame: %u) ==================\n", mCurFrameNo);
        DBG_LOG("Duration = %f\n", duration);
        DBG_LOG("Frame cnt = %d, FPS = %f\n", numOfFrames, fps);
        result["fps"] = fps;
    } else {
        DBG_LOG("Never rendered anything.\n");
        numOfFrames = 0;
        duration = 0;
        result["fps"] = 0;
    }

    result["time"] = duration;
    result["frames"] = numOfFrames;
    result["start_time"] = ((double)mTimerBeginTime) / os::timeFrequency;
    result["end_time"] = ((double)endTime) / os::timeFrequency;
    result["patrace_version"] = PATRACE_VERSION;
    if (mOptions.mPerfmon) perfmon_end(result);

    if (mCollectors)
    {
        mCollectors->stop();
    }
    DBG_LOG("Saving results...\n");
    if (!TraceExecutor::writeData(result, numOfFrames, duration))
    {
        reportAndAbort("Error writing result file!");
    }
    TraceExecutor::clearResult();
}

void Retracer::initializeCallCounter()
{
    mCallCounter["glLinkProgram"] = 0;
}

void Retracer::createWorkThreadPool()
{
    workThreadPoolMutex.lock();
    workThreadPool.clear();
    workThreadPoolMutex.unlock();
}

void Retracer::waitWorkThreadPoolIdle()
{
    wakeupAllWorkThreads();
    workThreadPoolMutex.lock();
    WorkThreadPool_t::iterator it;
    for (it = workThreadPool.begin(); it != workThreadPool.end(); it++)
    {
        WorkThread* wThread = it->second;
        workThreadPoolMutex.unlock();
        wThread->waitIdle();
        workThreadPoolMutex.lock();
    }
    workThreadPoolMutex.unlock();
}

void Retracer::destroyWorkThreadPool()
{
    workThreadPoolMutex.lock();
    WorkThreadPool_t::iterator it;
    for (it = workThreadPool.begin(); it != workThreadPool.end(); it++)
    {
        WorkThread* wThread = it->second;
        workThreadPool.erase(it);
        workThreadPoolMutex.unlock();
        wThread->waitIdle();
        wThread->setStatus(WorkThread::TERMINATED);
        wThread->workQueueWakeup();
        wThread->waitUntilExit();
        workThreadPoolMutex.lock();
        delete wThread;
    }
    workThreadPoolMutex.unlock();
}

WorkThread* Retracer::addWorkThread(unsigned tid)
{
    WorkThread* wThread = NULL;
    workThreadPoolMutex.lock();
    wThread = new WorkThread(&mFile);
    workThreadPool[tid] = wThread;
    wThread->start();
    wThread->waitIdle();
    workThreadPoolMutex.unlock();
    return wThread;
}

WorkThread* Retracer::findWorkThread(unsigned tid)
{
    WorkThread* wThread = NULL;
    workThreadPoolMutex.lock();
    WorkThreadPool_t::iterator it = workThreadPool.find(tid);
    if (it != workThreadPool.end())
    {
        wThread = it->second;
    }
    else
    {
        workThreadPoolMutex.unlock();
        wThread = addWorkThread(tid);
        workThreadPoolMutex.lock();
    }
    workThreadPoolMutex.unlock();

    return wThread;
}

void Retracer::wakeupAllWorkThreads()
{
    WorkThreadPool_t::iterator it;
    workThreadPoolMutex.lock();
    for (it = workThreadPool.begin(); it != workThreadPool.end(); it++)
    {
        WorkThread* wThread = it->second;
        wThread->workQueueWakeup();
    }
    workThreadPoolMutex.unlock();
}

void Retracer::DispatchWork(int tid, unsigned frameId, int callID, void* fptr, char* src, const char* name, common::UnCompressedChunk *chunk)
{
    if (!mOptions.mMultiThread)
    {
        if (mOptions.mCallStats && frameId >= mOptions.mBeginMeasureFrame && frameId < mOptions.mEndMeasureFrame)
        {
            uint64_t pre = gettime();
            (*(RetraceFunc)fptr)(src);
            uint64_t post = gettime();
            mCallStats[name].count++;
            mCallStats[name].time += post - pre;
        }
        else
        {
            (*(RetraceFunc)fptr)(src);
        }

        // Error Check
        if (mOptions.mDebug && hasCurrentContext())
        {
            CheckGlError();
        }
    }
    else
    {
        Work* work = new Work(tid, frameId, callID, fptr, src, name, chunk);
        if (mOptions.mCallStats && frameId >= mOptions.mBeginMeasureFrame && frameId < mOptions.mEndMeasureFrame)
        {
             work->SetMeasureTime(true);
        }
        DispatchWork(work);
    }
}

void Retracer::DispatchWork(Work* work)
{
    if (work == NULL)
        return;

    work->retain();
    if (mOptions.mMultiThread)
    {
        WorkThread* wThread = findWorkThread(work->getThreadID());
        if (wThread)
        {
            if (preThread != wThread)
            {
                if (preThread != NULL)
                {
                    preThread->waitIdle();
                }
                preThread = wThread;
            }
            wThread->workQueuePush(work);
        }
        else
        {
            DBG_LOG("Error:Can't create work thread for tid %d.\n", work->getThreadID());
            exit(-1);
        }
    }
    work->release();
}

void pre_glDraw()
{
    if (!gRetracer.mOptions.mDoOverrideResolution)
    {
        return;
    }

    Context& context = gRetracer.getCurrentContext();

    GLuint curFB = context._current_framebuffer;
    Rectangle& curAppVP = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppVP;
    Rectangle& curAppSR = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppSR;
    Rectangle& curDrvVP = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurDrvVP;
    Rectangle& curDrvSR = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurDrvSR;

    retracer::Drawable * curDrawable = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getDrawable();

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    if (curFB > 1)
#else
    if (curFB != 0)
#endif
    {
        // a framebuffer object is bound
        // only call glViewport and glScissor if appVP is different from drvVp

        if (curAppVP != curDrvVP) {
            curDrvVP = curAppVP;
            glViewport(curDrvVP.x, curDrvVP.y, curDrvVP.w, curDrvVP.h);
        }
        if (curAppSR != curDrvSR) {
            curDrvSR = curAppSR;
            glScissor(curDrvSR.x, curDrvSR.y, curDrvSR.w, curDrvSR.h);
        }
    }
    else
    {
        // on-screen framebuffer is "bound"

        if (curDrvVP != curAppVP.Stretch(curDrawable->mOverrideResRatioW, curDrawable->mOverrideResRatioH)) {
            curDrvVP = curAppVP.Stretch(curDrawable->mOverrideResRatioW, curDrawable->mOverrideResRatioH);
            glViewport(curDrvVP.x, curDrvVP.y, curDrvVP.w, curDrvVP.h);
        }
        if (curDrvSR != curAppSR.Stretch(curDrawable->mOverrideResRatioW, curDrawable->mOverrideResRatioH)) {
            curDrvSR = curAppSR.Stretch(curDrawable->mOverrideResRatioW, curDrawable->mOverrideResRatioH);
            glScissor(curDrvSR.x, curDrvSR.y, curDrvSR.w, curDrvSR.h);
        }
    }
}

void post_glCompileShader(GLuint shader, GLuint originalShaderName)
{
    GLint rvalue;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &rvalue);
    if (rvalue == GL_FALSE)
    {
        GLint maxLength = 0, len = -1;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
        char *infoLog = NULL;
        if (maxLength > 0)
        {
            infoLog = (char *)malloc(maxLength);
            glGetShaderInfoLog(shader, maxLength, &len, infoLog);
        }
        DBG_LOG("Error in compiling shader %u: %s\n", originalShaderName, infoLog ? infoLog : "(n/a)");
        free(infoLog);
    }
}

void post_glLinkProgram(GLuint program, GLuint originalProgramName, int status)
{
    GLint linkStatus;
    _glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_TRUE && status == 0)
    {
        // A bit of an odd case: Shader failed during tracing, but succeeds during replay. This can absolutely
        // happen without being an error (because feature checks can resolve differently on different platforms),
        // but warn about it anyway, because it _may_ give unexpected results in some cases.
        DBG_LOG("There was error in linking program %u in trace, but none on replay\n", program);
    }
    else if (linkStatus == GL_FALSE && (status == -1 || status == 1))
    {
        GLint infoLogLength;
        GLint len = -1;
        char *infoLog = (char *)NULL;
        _glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0)
        {
            infoLog = (char *)malloc(infoLogLength);
            glGetProgramInfoLog(program, infoLogLength, &len, infoLog);
        }
        vector<unsigned int>::iterator result = find(gRetracer.mOptions.mLinkErrorWhiteListCallNum.begin(), gRetracer.mOptions.mLinkErrorWhiteListCallNum.end(), gRetracer.GetCurCallId());
        if(result != gRetracer.mOptions.mLinkErrorWhiteListCallNum.end())
        {
            DBG_LOG("Error in linking program %d: %s. But this call has already been added to whitelist. So ignore this error and continue retracing.\n", originalProgramName, infoLog ? infoLog : "(n/a)");
        }
        else
        {
            gRetracer.reportAndAbort("Error in linking program %d: %s\n", originalProgramName, infoLog ? infoLog : "(n/a)");
        }
        free(infoLog);
    }

    if (!(gRetracer.mOptions.mStoreProgramInformation || gRetracer.mOptions.mRemoveUnusedVertexAttributes))
    {
        return;
    }

    Context& context = gRetracer.getCurrentContext();
    retracer::hmap<unsigned int>& shaderRevMap = context.getShaderRevMap();

    Json::Value& result = TraceExecutor::addProgramInfo(program, originalProgramName, shaderRevMap);

    if (gRetracer.mOptions.mRemoveUnusedVertexAttributes)
    {
        ShaderMod shaderMod(gRetracer, program, result, shaderRevMap);
        shaderMod.removeUnusedAttributes();

        if (shaderMod.getError())
        {
            DBG_LOG("ERROR: %s\n", shaderMod.getErrorString().c_str());
            TraceExecutor::addError(TRACE_ERROR_INCONSISTENT_TRACE_FILE, shaderMod.getErrorString());
        }
    }

    ++gRetracer.mCallCounter["glLinkProgram"];
}


void hardcode_glBindFramebuffer(int target, unsigned int framebuffer)
{
    gRetracer.getCurrentContext()._current_framebuffer = framebuffer;

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    const unsigned int ON_SCREEN_FBO = 1;
#else
    const unsigned int ON_SCREEN_FBO = 0;
#endif

    if (gRetracer.mOptions.mForceOffscreen && framebuffer == ON_SCREEN_FBO)
    {
        gRetracer.mpOffscrMgr->BindOffscreenFBO();
    }
    else
    {
        glBindFramebuffer(target, framebuffer);
    }
}

void hardcode_glDeleteBuffers(int n, unsigned int* oldIds)
{
    Context& context = gRetracer.getCurrentContext();

    hmap<unsigned int>& idMap = context.getBufferMap();
    hmap<unsigned int>& idRevMap = context.getBufferRevMap();
    for (int i = 0; i < n; ++i)
    {
        unsigned int oldId = oldIds[i];
        unsigned int newId = idMap.RValue(oldId);
        glDeleteBuffers(1, &newId);
        idMap.LValue(oldId) = 0;
        idRevMap.LValue(newId) = 0;
    }
}

void hardcode_glDeleteFramebuffers(int n, unsigned int* oldIds)
{
    Context& context = gRetracer.getCurrentContext();

    hmap<unsigned int>& idMap = context._framebuffer_map;
    for (int i = 0; i < n; ++i)
    {
        unsigned int oldId = oldIds[i];
        unsigned int newId = idMap.RValue(oldId);

        GLint preReadFboId, preDrawFboId;
        _glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &preReadFboId);
        _glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &preDrawFboId);

        glDeleteFramebuffers(1, &newId);
        idMap.LValue(oldId) = 0;

        if (gRetracer.mOptions.mForceOffscreen &&
            (newId == (unsigned int)preReadFboId || newId == (unsigned int)preDrawFboId))
        {
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
            const unsigned int ON_SCREEN_FBO = 1;
#else
            const unsigned int ON_SCREEN_FBO = 0;
#endif
            gRetracer.getCurrentContext()._current_framebuffer = ON_SCREEN_FBO;
            gRetracer.mpOffscrMgr->BindOffscreenFBO();
        }
    }
}

void hardcode_glDeleteRenderbuffers(int n, unsigned int* oldIds)
{
    Context& context = gRetracer.getCurrentContext();

    hmap<unsigned int>& idMap = context.getRenderbufferMap();
    hmap<unsigned int>& idRevMap = context.getRenderbufferRevMap();
    for (int i = 0; i < n; ++i)
    {
        unsigned int oldId = oldIds[i];
        unsigned int newId = idMap.RValue(oldId);
        glDeleteRenderbuffers(1, &newId);
        idMap.LValue(oldId) = 0;
        idRevMap.LValue(newId) = 0;
    }
}

void hardcode_glDeleteTextures(int n, unsigned int* oldIds)
{
    Context& context = gRetracer.getCurrentContext();

    hmap<unsigned int>& idMap = context.getTextureMap();
    hmap<unsigned int>& idRevMap = context.getTextureRevMap();
    for (int i = 0; i < n; ++i)
    {
        unsigned int oldId = oldIds[i];
        unsigned int newId = idMap.RValue(oldId);
        glDeleteTextures(1, &newId);
        idMap.LValue(oldId) = 0;
        idRevMap.LValue(newId) = 0;
    }
}

void hardcode_glDeleteTransformFeedbacks(int n, unsigned int* oldIds)
{
    Context& context = gRetracer.getCurrentContext();

    hmap<unsigned int>& idMap = context._feedback_map;
    for (int i = 0; i < n; ++i)
    {
        unsigned int oldId = oldIds[i];
        unsigned int newId = idMap.RValue(oldId);
        glDeleteTransformFeedbacks(1, &newId);
        idMap.LValue(oldId) = 0;
    }
}

void hardcode_glDeleteQueries(int n, unsigned int* oldIds)
{
    Context& context = gRetracer.getCurrentContext();

    hmap<unsigned int>& idMap = context._query_map;
    for (int i = 0; i < n; ++i)
    {
        unsigned int oldId = oldIds[i];
        unsigned int newId = idMap.RValue(oldId);
        glDeleteQueries(1, &newId);
        idMap.LValue(oldId) = 0;
    }
}

void hardcode_glDeleteSamplers(int n, unsigned int* oldIds)
{
    Context& context = gRetracer.getCurrentContext();

    hmap<unsigned int>& idMap = context.getSamplerMap();
    hmap<unsigned int>& idRevMap = context.getSamplerRevMap();
    for (int i = 0; i < n; ++i)
    {
        unsigned int oldId = oldIds[i];
        unsigned int newId = idMap.RValue(oldId);
        glDeleteSamplers(1, &newId);
        idMap.LValue(oldId) = 0;
        idRevMap.LValue(newId) = 0;
    }
}

void hardcode_glDeleteVertexArrays(int n, unsigned int* oldIds)
{
    Context& context = gRetracer.getCurrentContext();

    hmap<unsigned int>& idMap = context._array_map;
    for (int i = 0; i < n; ++i)
    {
        unsigned int oldId = oldIds[i];
        unsigned int newId = idMap.RValue(oldId);
        glDeleteVertexArrays(1, &newId);
        idMap.LValue(oldId) = 0;
    }
}

GLuint lookUpPolymorphic(GLuint name, GLenum target)
{
    Context& context = gRetracer.getCurrentContext();
    if (target == GL_RENDERBUFFER)
    {
        return context.getRenderbufferMap().RValue(name);
    }
    // otherwise, assume texture type
    return context.getTextureMap().RValue(name);
}

} /* namespace retracer */
