#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "retracer/retracer.hpp"

#include "retracer/glstate.hpp"
#include "retracer/retrace_api.hpp"
#include "retracer/trace_executor.hpp"
#include "retracer/forceoffscreen/offscrmgr.h"
#include "retracer/glws.hpp"
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

#include "json/writer.h"
#include "json/reader.h"

#include <chrono>
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
#include <sys/time.h>
#include <sys/resource.h>

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
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
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
            fclose(pm_fp);
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
    if (pm_fp) fprintf(pm_fp, "run");
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
    if (pm_fp) fprintf(pm_fp, "\n");
    if (pm_fp) fclose(pm_fp);
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

Retracer::~Retracer()
{
    delete mCollectors;

#ifndef NDEBUG
    if (mVBODataSize) DBG_LOG("VBO data size : %d\n", mVBODataSize);
    if (mTextureDataSize) DBG_LOG("Uncompressed texture data size : %d\n", mTextureDataSize);
    if (mCompressedTextureDataSize) DBG_LOG("Compressed texture data size : %d\n", mCompressedTextureDataSize);
    if (mClientSideMemoryDataSize) DBG_LOG("Client-side memory data size : %d\n", mClientSideMemoryDataSize);
#endif
}

bool Retracer::OpenTraceFile(const char* filename)
{
    if (!mFile.Open(filename))
        return false;

    mFileFormatVersion = mFile.getHeaderVersion();
    mStateLogger.open(std::string(filename) + ".retracelog");
    loadRetraceOptionsFromHeader();
    mFinish.store(false);
    initializeCallCounter();

    return true;
}

__attribute__ ((noinline)) static int noop(int a)
{
    return a + 1;
}

void Retracer::CloseTraceFile()
{
    mFile.Close();
    mFileFormatVersion = INVALID_VERSION;
    mStateLogger.close();
    mState.Reset();
    mCSBuffers.clear();
    mSnapshotPaths.clear();

    if (shaderCacheFile)
    {
        fclose(shaderCacheFile);
        shaderCacheFile = NULL;
    }
}

bool Retracer::loadRetraceOptionsByThreadId(int tid)
{
    const Json::Value jsThread = mFile.getJSONThreadById(tid);
    if (jsThread.isNull())
    {
        DBG_LOG("No stored EGL config for this tid: %d\n", tid);
        return false;
    }
    if (mOptions.mWindowWidth == 0) mOptions.mWindowWidth = jsThread["winW"].asInt();
    if (mOptions.mWindowHeight == 0) mOptions.mWindowHeight = jsThread["winH"].asInt();
    mOptions.mOnscreenConfig = jsThread["EGLConfig"];
    mOptions.mOffscreenConfig = jsThread["EGLConfig"];
    return true;
}

void Retracer::loadRetraceOptionsFromHeader()
{
    // Load values from headers first, then any valid commandline parameters override the header defaults.
    const Json::Value jsHeader = mFile.getJSONHeader();
    if (mOptions.mRetraceTid == -1) mOptions.mRetraceTid = jsHeader.get("defaultTid", -1).asInt();
    if (mOptions.mRetraceTid == -1) reportAndAbort("No thread ID set!");
    if (jsHeader.isMember("forceSingleWindow")) mOptions.mForceSingleWindow = jsHeader.get("forceSingleWindow", false).asBool();
    if (jsHeader.isMember("singleSurface")) mOptions.mSingleSurface = jsHeader.get("singleSurface", false).asInt();
    if (mOptions.mForceSingleWindow && mOptions.mSingleSurface != -1) reportAndAbort("forceSingleWindow and singleSurface cannot be used together");
    if (mOptions.mForceSingleWindow) DBG_LOG("Enabling force single window option\n");
    if (jsHeader.isMember("multiThread")) mOptions.mMultiThread = jsHeader.get("multiThread", false).asBool();
    if (mOptions.mMultiThread) DBG_LOG("Enabling multiple thread option\n");
    if (jsHeader.isMember("skipfence")) {
        std::vector<std::pair<unsigned int, unsigned int>> ranges;
        for (auto ranges_itr : jsHeader["skipfence"])
        {
            ranges.push_back(std::make_pair(ranges_itr[0].asInt(), ranges_itr[1].asInt()));
        }

        if (ranges.size() == 0)
        {
            gRetracer.reportAndAbort("Bad value for option -skipfence, must give at least one frame range.");
        }

        std::sort(ranges.begin(), ranges.end());

        unsigned int start = ranges[0].first;
        unsigned int end = ranges[0].second;

        std::vector<std::pair<unsigned int, unsigned int>> merged_ranges;
        for (unsigned int i = 0; i < ranges.size() - 1; ++i)
        {
            if (ranges[i].second >= ranges[i + 1].first)
            {
                if (ranges[i].second < ranges[i + 1].second)
                {
                    end = ranges[i + 1].second;
                }
                else
                {
                    i += 1;
                }
            }
            else
            {
                merged_ranges.push_back(std::make_pair(start, end));
                start = ranges[i + 1].first;
                end = ranges[i + 1].second;
            }
        }

        if (merged_ranges.size() > 0)
        {
            mOptions.mSkipFence = true;
            mOptions.mSkipFenceRanges = merged_ranges;
        }
        else
        {
            mOptions.mSkipFence = false;
        }
    }
    if (mOptions.mSkipFence) DBG_LOG("Enabling fence skip option\n");
    switch (jsHeader["glesVersion"].asInt())
    {
    case 1: mOptions.mApiVersion = PROFILE_ES1; break;
    case 2: mOptions.mApiVersion = PROFILE_ES2; break;
    case 3: mOptions.mApiVersion = PROFILE_ES3; break;
    default: DBG_LOG("Error: Invalid glesVersion parameter\n"); break;
    }
    loadRetraceOptionsByThreadId(mOptions.mRetraceTid);
    const Json::Value& linkErrorWhiteListCallNum = jsHeader["linkErrorWhiteListCallNum"];
    for(unsigned int i=0; i<linkErrorWhiteListCallNum.size(); i++)
    {
        mOptions.mLinkErrorWhiteListCallNum.push_back(linkErrorWhiteListCallNum[i].asUInt());
    }
    if (mOptions.mForceOffscreen)
    {
        // When running offscreen, force onscreen EGL to most compatible mode known: 5650 00
        mOptions.mOnscreenConfig = EglConfigInfo(5, 6, 5, 0, 0, 0, 0, 0);
        mOptions.mOffscreenConfig.override(mOptions.mOverrideConfig);
    }
    else
    {
        mOptions.mOnscreenConfig.override(mOptions.mOverrideConfig);
    }

    const int required_major = jsHeader.get("required_replayer_version_major", 0).asInt();
    const int required_minor = jsHeader.get("required_replayer_version_minor", 0).asInt();
    if (required_major > PATRACE_VERSION_MAJOR || (required_major == PATRACE_VERSION_MAJOR && required_minor > PATRACE_VERSION_MINOR))
    {
        reportAndAbort("Required replayer version is r%dp%d, your version is r%dp%d", required_major, required_minor, PATRACE_VERSION_MAJOR, PATRACE_VERSION_MINOR);
    }
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
        Texture t = {};
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
                gRetracer.mpOffscrMgr->BindOffscreenReadFBO();
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
                if (mOptions.mSnapshotFrameNames || mOptions.mLoopTimes > 0 || mOptions.mLoopSeconds > 0)
                {
                    ss << mOptions.mSnapshotPrefix << std::setw(4) << std::setfill('0') << frameNo << "_l" << mLoopTimes << ".png";
                }
                else // use classic weird name
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

// Only one thread runs at a time, so no need for mutexing etc. except for when we go to sleep.
void Retracer::RetraceThread(const int threadidx, const int our_tid)
{
    std::unique_lock<std::mutex> lk(mConditionMutex);
    thread_result r;
    r.our_tid = our_tid;
    unsigned int skip_fence_range_index = 0;

    while (!mFinish.load(std::memory_order_consume))
    {
        const bool isSwapBuffers = swapvals[mCurCall.funcId];

        if (mOptions.mSnapshotCallSet && (mOptions.mSnapshotCallSet->contains(mCurFrameNo, mFile.ExIdToName(mCurCall.funcId))) && isSwapBuffers)
        {
            TakeSnapshot(mFile.curCallNo - 1, mCurFrameNo);
        }

        if (fptr)
        {
            r.total++;
            if (isSwapBuffers)
            {
                // call glFinish() before eglSwapbuffers in every frame when mFinishBeforeSwap is true or in frame#0 of a FF trace
                if (mOptions.mFinishBeforeSwap || (mCurFrameNo == 0 && mFile.isFFTrace()))
                {
                    _glFinish();
                }

                if (mOptions.mSkipFence && ((mCurFrameNo + 1) > mOptions.mSkipFenceRanges[skip_fence_range_index].second))
                {
                    skip_fence_range_index += 1;

                    if (skip_fence_range_index >= mOptions.mSkipFenceRanges.size())
                    {
                        mOptions.mSkipFence = false;
                    }
                }
            }

            if (mOptions.mDebug > 1) DBG_LOG("    %s: t%d, c%d, f%d \n", mFile.ExIdToName(mCurCall.funcId), our_tid, mFile.curCallNo, mCurFrameNo);

            if (mOptions.mSkipFence && syncvals[mCurCall.funcId] && (mCurFrameNo >= mOptions.mSkipFenceRanges[skip_fence_range_index].first) && (mCurFrameNo <= mOptions.mSkipFenceRanges[skip_fence_range_index].second))
            {
                if (mOptions.mDebug) DBG_LOG("    FENCE SKIP : function name: %s (id: %d), call no: %d\n", mFile.ExIdToName(mCurCall.funcId), mCurCall.funcId, mFile.curCallNo);
            }
            else if (mOptions.mCallStats && mCurFrameNo >= mOptions.mBeginMeasureFrame && mCurFrameNo < mOptions.mEndMeasureFrame)
            {
                const char *funcName = mFile.ExIdToName(mCurCall.funcId);
                uint64_t pre = gettime();
                (*(RetraceFunc)fptr)(src);
                uint64_t post = gettime();
                mCallStats[funcName].count++;
                mCallStats[funcName].time += post - pre;
            }
            else if (!mOptions.mCacheOnly || cachevals[mCurCall.funcId])
            {
                (*(RetraceFunc)fptr)(src);
            }

            // Error Check
            if (mOptions.mDebug && hasCurrentContext())
            {
                CheckGlError();
            }
            if (isSwapBuffers && (mCurCall.tid == mOptions.mRetraceTid || mOptions.mMultiThread))
            {
                if (mOptions.mPerfmon) perfmon_frame();
            }
            r.swaps += (int)isSwapBuffers;
        }
        else if (mOptions.mDebug && mOptions.mRunAll)
        {
            DBG_LOG("    Unsupported function : %s, call no: %d\n", mFile.ExIdToName(mCurCall.funcId), mFile.curCallNo);
        }

        if (isSwapBuffers)
        {
            if (mOptions.mPerfStart == (int)mCurFrameNo) // before perf frame
            {
                PerfStart();
            }
            else if (mOptions.mPerfStop == (int)mCurFrameNo) // last frame
            {
                PerfEnd();
            }

            if (mOptions.mScriptFrame == (int)mCurFrameNo && mOptions.mScriptPath.size() > 0)  // trigger script at the begining of specific frame
            {
                TriggerScript(mOptions.mScriptPath.c_str());
            }

            if (mOptions.mDebug)
            {
                const long pages = sysconf(_SC_AVPHYS_PAGES);
                const long page_size = sysconf(_SC_PAGE_SIZE);
                const long available = pages * page_size;
                struct rusage usage;
                getrusage(RUSAGE_SELF, &usage);
                long curr_rss = -1;
                FILE* fp = NULL;
                if ((fp = fopen( "/proc/self/statm", "r" )))
                {
                    if (fscanf(fp, "%*s%ld", &curr_rss) == 1)
                    {
                        curr_rss *= page_size;
                    }
                    fclose(fp);
                }
                const double f = 1024.0 * 1024.0;
                DBG_LOG("Frame %d memory (mb): %.02f max RSS, %.02f current RSS, %.02f available, %.02f client side memory, %.02f loaded file data\n",
                        mCurFrameNo, (double)usage.ru_maxrss / 1024.0, (double)curr_rss / f, (double)available / f, (double)mClientSideMemoryDataSize / f, (double)mFile.memoryUsed() / f);
            }

            const int secs = (os::getTime() - mTimerBeginTime) / os::timeFrequency;
            if (mCurFrameNo >= mOptions.mEndMeasureFrame && (mOptions.mLoopTimes > mLoopTimes || (mOptions.mLoopSeconds > 0 && secs < mOptions.mLoopSeconds)))
            {
                DBG_LOG("Executing rollback %d / %d times - %d / %d secs\n", mLoopTimes, mOptions.mLoopTimes, secs, mOptions.mLoopSeconds);
                if (mCollectors) mCollectors->summarize();
                mFile.rollback();
                unsigned numOfFrames = mCurFrameNo - mOptions.mBeginMeasureFrame;
                mCurFrameNo = mOptions.mBeginMeasureFrame;
                mFile.curCallNo = mRollbackCallNo;
                int64_t endTime;
                const float duration = getDuration(mLoopBeginTime, &endTime);
                const float fps = ((double)numOfFrames) / duration;
                mLoopResults.push_back(fps);
                mLoopBeginTime = os::getTime();
                mLoopTimes++;
            }
        }
        else if (mOptions.mSnapshotCallSet && (mOptions.mSnapshotCallSet->contains(mFile.curCallNo, mFile.ExIdToName(mCurCall.funcId))))
        {
            TakeSnapshot(mFile.curCallNo, mCurFrameNo);
        }

        while (frameBudget <= 0 && drawBudget <= 0) // Step mode
        {
            frameBudget = 0;
            drawBudget = 0;
            StepShot(mFile.curCallNo, mCurFrameNo);
            GLWS::instance().processStepEvent(); // will wait here for user input to increase budgets
        }

        // ---------------------------------------------------------------------------
        // Get next call
skip_call:

        if (!mFile.GetNextCall(fptr, mCurCall, src))
        {
            mFinish.store(true);
            for (auto &cv : conditions) cv.notify_one(); // Wake up all other threads
            break;
        }
        // Skip call because it is on an ignored thread?
        if (!mOptions.mMultiThread && mCurCall.tid != mOptions.mRetraceTid)
        {
            r.skipped++;
            goto skip_call;
        }
        // Need to switch active thread?
        if (our_tid != mCurCall.tid)
        {
            latest_call_tid = mCurCall.tid; // need to use an atomic member copy of this here
            // Do we need to make this thread?
            if (thread_remapping.count(mCurCall.tid) == 0)
            {
                thread_remapping[mCurCall.tid] = threads.size();
                int newthreadidx = threads.size();
                conditions.emplace_back();
                results.emplace_back();
                threads.emplace_back(&Retracer::RetraceThread, this, (int)newthreadidx, (int)mCurCall.tid);
            }
            else // Wake up existing thread
            {
                const int otheridx = thread_remapping.at(mCurCall.tid);
                conditions.at(otheridx).notify_one();
            }
            r.handovers++;
            bool success = false;
            do {
                success = conditions.at(threadidx).wait_for(lk, std::chrono::milliseconds(50), [&]{ return our_tid == latest_call_tid || mFinish.load(std::memory_order_consume); });
                if (!success) r.timeouts++; else r.wakeups++;
            } while (!success);
        }
    }
    results[threadidx] = r;
}

void Retracer::Retrace()
{
    if (!mOptions.mCpuMask.empty()) set_cpu_mask(mOptions.mCpuMask);
    report_cpu_mask();

    //pre-process of shader cache file if needed
    if (gRetracer.mOptions.mShaderCacheFile.size() > 0)
    {
        if (gRetracer.mOptions.mShaderCacheLoad)
            OpenShaderCacheFile();
        else
            DeleteShaderCacheFile();
    }

    mFile.setFrameRange(mOptions.mBeginMeasureFrame, mOptions.mEndMeasureFrame, mOptions.mMultiThread ? -1 : mOptions.mRetraceTid, mOptions.mPreload, mOptions.mLoopTimes != 0);

    mInitTime = os::getTime();
    mInitTimeMono = os::getTimeType(CLOCK_MONOTONIC);
    mInitTimeMonoRaw = os::getTimeType(CLOCK_MONOTONIC_RAW);
    mInitTimeBoot = os::getTimeType(CLOCK_BOOTTIME);

    swapvals.resize(mFile.getMaxSigId());
    swapvals[mFile.NameToExId("eglSwapBuffers")] = true;
    swapvals[mFile.NameToExId("eglSwapBuffersWithDamageKHR")] = true;
    cachevals.resize(mFile.getMaxSigId());
    for (const auto& s : mFile.getFuncNames())
    {
        if (s.find("Uniform") != string::npos || s.find("Attrib") != string::npos || s.find("Shader") != string::npos || s.find("Program") != string::npos || (s[0] == 'e' && s[1] == 'g' && s[2] == 'l')
            || s.find("Feedback") != string::npos || s.find("Buffer") != string::npos)
        {
            cachevals[mFile.NameToExId(s.c_str())] = true;
        }
    }
    syncvals.resize(mFile.getMaxSigId());
    syncvals[mFile.NameToExId("eglClientWaitSync")] = true;
    syncvals[mFile.NameToExId("eglClientWaitSyncKHR")] = true;
    syncvals[mFile.NameToExId("eglWaitSync")] = true;
    syncvals[mFile.NameToExId("eglWaitSyncKHR")] = true;
    syncvals[mFile.NameToExId("glWaitSync")] = true;
    syncvals[mFile.NameToExId("glClientWaitSync")] = true;

    if (mOptions.mScriptFrame == 0 && mOptions.mScriptPath.size() > 0)
    {
        TriggerScript(mOptions.mScriptPath.c_str());
    }

    if (mOptions.mBeginMeasureFrame == 0 && mCurFrameNo == 0)
    {
        StartMeasuring(); // special case, otherwise triggered by eglSwapBuffers()
        delayedPerfmonInit = true;
    }

    // Get first packet on a relevant thread
    do
    {
        if (!mFile.GetNextCall(fptr, mCurCall, src) || mFinish.load(std::memory_order_consume))
        {
            reportAndAbort("Empty trace file!");
        }
    } while (!mOptions.mMultiThread && mCurCall.tid != mOptions.mRetraceTid);
    threads.resize(1);
    conditions.resize(1);
    results.resize(1);
    thread_remapping[mCurCall.tid] = 0;
    results[0].our_tid = mCurCall.tid;
    RetraceThread(0, mCurCall.tid); // run the first thread on this thread

    for (std::thread &t : threads)
    {
        if (t.joinable()) t.join();
    }

    // When we get here, we're all done
    if (mOptions.mForceOffscreen)
    {
        forceRenderMosaicToScreen();
    }

    if (GetGLESVersion() > 1)
    {
        GLWS::instance().MakeCurrent(gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getDrawable(),
                                     gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext());
        _glFinish();
    }
}

void Retracer::CheckGlError()
{
    GLenum error = glGetError();
    if (error == GL_NO_ERROR)
    {
        return;
    }
    DBG_LOG("[%d] %s  ERR: %d \n", GetCurCallId(), GetCurCallName(), error);
}

void Retracer::OnFrameComplete()
{
    if (getCurTid() == mOptions.mRetraceTid || mOptions.mMultiThread)
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
    if (mOptions.mCollectorEnabled)
    {
        DBG_LOG("libcollector enabled.\n");
        mOptions.mCollectorEnabled = false;
        mCollectors = new Collection(mOptions.mCollectorValue);
        mCollectors->initialize();
        mCollectors->start();
    }
    mRollbackCallNo = mFile.curCallNo;
    DBG_LOG("================== Start timer (Frame: %u) ==================\n", mCurFrameNo);
    mTimerBeginTime = mLoopBeginTime = os::getTime();
    mTimerBeginTimeMono = os::getTimeType(CLOCK_MONOTONIC);
    mTimerBeginTimeMonoRaw = os::getTimeType(CLOCK_MONOTONIC_RAW);
    mTimerBeginTimeBoot = os::getTimeType(CLOCK_BOOTTIME);
    mEndFrameTime = mTimerBeginTime;
}

void Retracer::OnNewFrame()
{
    if (getCurTid() == mOptions.mRetraceTid || mOptions.mMultiThread)
    {
        IncCurFrameId();

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
            if (mOptions.mInstrumentationDelay > 0) {
                usleep(mOptions.mInstrumentationDelay);
            }
            mCollectors->collect();
        }
    }
}

void Retracer::TriggerScript(const char* scriptPath)
{
    char cmd[256];
    memset(cmd, 0, sizeof(cmd));
#ifdef ANDROID
    sprintf(cmd, "/system/bin/sh %s", scriptPath);
#else
    sprintf(cmd, "/bin/sh %s", scriptPath);
#endif

    int ret = system(cmd);

    DBG_LOG("Trigger script %s run result: %d\n", cmd, ret);
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
#ifdef ANDROID
        std::string freqopt = _to_string(mOptions.mPerfFreq);
        std::string mypid = _to_string(parent);
        const char* args[10] = { mOptions.mPerfPath.c_str(), "record", "-g", "-f", freqopt.c_str(), "-p", mypid.c_str(), "-o", mOptions.mPerfOut.c_str(), nullptr };
        DBG_LOG("Perf tracing %ld from process %ld with freq %ld and output in %s\n", (long)parent, (long)getpid(), (long)mOptions.mPerfFreq, mOptions.mPerfOut.c_str());
        DBG_LOG("Perf args: %s %s %s %s %s %s %s %s %s\n", args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
#else
        std::string freqopt = "--freq=" + _to_string(mOptions.mPerfFreq);
        std::string mypid = "--pid=" + _to_string(parent);
        std::string myfilename = mOptions.mPerfOut;
        std::string myfilearg = "--output=" + myfilename;
        const char* args[7] = { mOptions.mPerfPath.c_str(), "record", "-g", freqopt.c_str(), mypid.c_str(), myfilearg.c_str(), nullptr };
        DBG_LOG("Perf tracing %ld from process %ld with freq %ld and output in %s\n", (long)parent, (long)getpid(), (long)mOptions.mPerfFreq, myfilename.c_str());
        DBG_LOG("Perf args: %s %s %s %s %s %s\n", args[0], args[1], args[2], args[3], args[4], args[5]);
#endif
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
    if(child == -1)
        return;
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
    sprintf(buf, "[c%u,f%u] ", GetCurCallId(), GetCurFrameId());
    va_start(ap, format);
    const unsigned len = strlen(buf);
    vsnprintf(buf + len, sizeof(buf) - len - 1, format, ap);
    va_end(ap);
    TraceExecutor::writeError(buf);
#ifdef __APPLE__
     throw PA_EXCEPTION(buf);
#elif ANDROID
    sleep(1); // give log call a chance before world dies below...
    ::exit(0); // too many failure calls will cause Android to hate us and blacklist our app until we uninstall it
#else
    ::abort();
#endif
}

void Retracer::saveResult(Json::Value& result)
{
    int64_t endTime;
    int64_t endTimeMono = os::getTimeType(CLOCK_MONOTONIC);
    int64_t endTimeMonoRaw = os::getTimeType(CLOCK_MONOTONIC_RAW);
    int64_t endTimeBoot = os::getTimeType(CLOCK_BOOTTIME);
    float duration = getDuration(mTimerBeginTime, &endTime);
    unsigned int numOfFrames = mCurFrameNo - mOptions.mBeginMeasureFrame;

    if(mTimerBeginTime != 0) {
        const float fps = ((double)numOfFrames * std::max(1, mLoopTimes)) / duration;
        const float loopDuration = getDuration(mLoopBeginTime, &endTime);
        const float loopFps = ((double)numOfFrames) / loopDuration;
        DBG_LOG("================== End timer (Frame: %u) ==================\n", mCurFrameNo);
        DBG_LOG("Duration = %f\n", duration);
        DBG_LOG("Frame cnt = %d, FPS = %f\n", numOfFrames, fps);
        result["fps"] = fps;
        mLoopResults.push_back(loopFps);
    } else {
        DBG_LOG("Never rendered anything.\n");
        numOfFrames = 0;
        duration = 0;
        result["fps"] = 0;
    }

    result["loopFPS"] = Json::arrayValue;
    for (const auto fps : mLoopResults) result["loopFPS"].append(fps);
    result["time"] = duration;
    result["frames"] = numOfFrames;
    result["init_time"] = ((double)mInitTime) / os::timeFrequency;
    result["start_time"] = ((double)mTimerBeginTime) / os::timeFrequency;
    result["end_time"] = ((double)endTime) / os::timeFrequency;
    result["start_frame"] = mOptions.mBeginMeasureFrame;
    result["end_frame"] = mOptions.mEndMeasureFrame;
    result["init_time_monotonic"] = ((double)mInitTimeMono) / os::timeFrequency;
    result["start_time_monotonic"] = ((double)mTimerBeginTimeMono) / os::timeFrequency;
    result["end_time_monotonic"] = ((double)endTimeMono) / os::timeFrequency;
    result["init_time_monotonic_raw"] = ((double)mInitTimeMonoRaw) / os::timeFrequency;
    result["start_time_monotonic_raw"] = ((double)mTimerBeginTimeMonoRaw) / os::timeFrequency;
    result["end_time_monotonic_raw"] = ((double)endTimeMonoRaw) / os::timeFrequency;
    result["init_time_boot"] = ((double)mInitTimeBoot) / os::timeFrequency;
    result["start_time_boot"] = ((double)mTimerBeginTimeBoot) / os::timeFrequency;
    result["end_time_boot"] = ((double)endTimeBoot) / os::timeFrequency;
    result["patrace_version"] = PATRACE_VERSION;
    if (mOptions.mPerfmon) perfmon_end(result);

    if (mCollectors)
    {
        mCollectors->stop();
    }

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
        FILE *fp = fopen(filename, "w");
        if (fp)
        {
            uint64_t total = 0;
            float noop_avg = (float)mCallStats["NO-OP"].time / mCallStats["NO-OP"].count;
            fprintf(fp, "Function,Calls,Time,Calibrated_Time\n");
            for (const auto& pair : mCallStats)
            {
                uint64_t noop = (uint64_t)(pair.second.count*noop_avg);
                uint64_t calibrated_time = (pair.second.time > noop) ? (pair.second.time - noop) : 0;
                fprintf(fp, "%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n", pair.first.c_str(), pair.second.count, pair.second.time, calibrated_time);
                // exclude APIs introduced from patrace
                if (strcmp(pair.first.c_str(), "glClientSideBufferData")   && strcmp(pair.first.c_str(), "glClientSideBufferSubData") &&
                    strcmp(pair.first.c_str(), "glCreateClientSideBuffer") && strcmp(pair.first.c_str(), "glDeleteClientSideBuffer") &&
                    strcmp(pair.first.c_str(), "glCopyClientSideBuffer")   && strcmp(pair.first.c_str(), "glPatchClientSideBuffer") &&
                    strcmp(pair.first.c_str(), "glGenGraphicBuffer_ARM")   && strcmp(pair.first.c_str(), "glGraphicBufferData_ARM") &&
                    strcmp(pair.first.c_str(), "glDeleteGraphicBuffer_ARM")&& strcmp(pair.first.c_str(), "NO-OP"))
                    total += calibrated_time;
            }
            fsync(fileno(fp));
            fclose(fp);

            const float ddk_fps = ((float)numOfFrames * std::max(1, mLoopTimes)) / ticksToSeconds(total);
            const float ddk_mspf = (1000 * ticksToSeconds(total)) / (float)numOfFrames;
            result["fps_ddk"] = ddk_fps;
            result["ms/frame_ddk"] = ddk_mspf;
            DBG_LOG("DDK FPS = %f, ms/frame = %f\n", ddk_fps, ddk_mspf);
            DBG_LOG("Writing callstats to %s\n", filename);
        }
        else
        {
            DBG_LOG("Failed to open output callstats in %s: %s\n", filename, strerror(errno));
        }
        mCallStats.clear();
    }

    DBG_LOG("Saving results...\n");
    if (!TraceExecutor::writeData(result, numOfFrames, duration))
    {
        reportAndAbort("Error writing result file!\n");
    }
    mLoopResults.clear();
    TraceExecutor::clearResult();

    if (mOptions.mDebug)
    {
        for (unsigned threadidx = 0; threadidx < results.size(); threadidx++)
        {
            thread_result& r = results.at(threadidx);
            DBG_LOG("Thread %d (%d):\n", threadidx, r.our_tid);
            DBG_LOG("\tTotal calls: %d\n", r.total);
            DBG_LOG("\tSkipped calls: %d\n", r.skipped);
            DBG_LOG("\tSwapbuffer calls: %d\n", r.swaps);
            DBG_LOG("\tHandovers: %d\n", r.handovers);
            DBG_LOG("\tWakeups: %d\n", r.wakeups);
            DBG_LOG("\tTimeouts: %d\n", r.timeouts);
        }
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
            return;
        }
        pid_t pid = getpid();
        std::string itAppName;
        std::string pname = getProcessNameByPid(getpid());
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
}

void Retracer::initializeCallCounter()
{
    mCallCounter["glLinkProgram"] = 0;
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

void post_glShaderSource(GLuint shader, GLuint originalShaderName, GLsizei count, const GLchar **string, const GLint *length)
{
    if (gRetracer.mOptions.mShaderCacheFile.size() > 0 && string && count)
    {
        std::string cat;
        for (int i = 0; i < count; i++)
        {
            if (length) cat += std::string(string[i], length[i]);
            else cat += string[i];
        }
        gRetracer.getCurrentContext().setShaderSource(shader, cat);
    }
}

void DeleteShaderCacheFile()
{
    const std::string bpath = gRetracer.mOptions.mShaderCacheFile + ".bin";
    remove(bpath.c_str());
}

void OpenShaderCacheFile()
{
        const std::string bpath = gRetracer.mOptions.mShaderCacheFile + ".bin";
        gRetracer.shaderCacheFile = fopen(bpath.c_str(), "rb");
        if (!gRetracer.shaderCacheFile)
        {
            gRetracer.reportAndAbort("Failed to open shader cache file %s: %s", bpath.c_str(), strerror(errno));
        }

        const std::string ipath = gRetracer.mOptions.mShaderCacheFile + ".idx";
        FILE *idx = fopen(ipath.c_str(), "rb");
        if (idx)
        {
            std::vector<char> ver_md5;
            ver_md5.resize(MD5Digest::DIGEST_LEN * 2);
            int r = fread(ver_md5.data(), ver_md5.size(), 1, idx);
            if (r != 1)
            {
                gRetracer.reportAndAbort("Failed to read shader cache version: %s", ipath.c_str(), strerror(ferror(idx)));
            }
            gRetracer.shaderCacheVersionMD5 = std::string(ver_md5.data(), ver_md5.size());

            uint32_t size = 0;
            r = fread(&size, sizeof(size), 1, idx);
            if (r != 1)
            {
                gRetracer.reportAndAbort("Failed to read shader cache index size: %s", ipath.c_str(), strerror(ferror(idx)));
            }
            for (unsigned i = 0; i < size; i++)
            {
                std::vector<char> md5;
                uint64_t offset = 0;
                std::string md5_str;
                md5.resize(MD5Digest::DIGEST_LEN * 2);
                r = fread(md5.data(), md5.size(), 1, idx);
                r += fread(&offset, sizeof(offset), 1, idx);
                if (r != 2)
                {
                    gRetracer.reportAndAbort("Failed to read shader cache index: %s", ipath.c_str(), strerror(ferror(idx)));
                }
                md5_str = std::string(md5.data(), md5.size());
                gRetracer.shaderCacheIndex[md5_str] = offset;

                if (offset == UINT64_MAX) continue;  // skipped shadercacheIndex

                GLenum binaryFormat = GL_NONE;
                uint32_t size = 0;

                if (fseek(gRetracer.shaderCacheFile, offset, SEEK_SET) != 0)
                {
                    gRetracer.reportAndAbort("Could not seek to desired cache item at %ld", offset);
                }
                if (fread(&binaryFormat, sizeof(binaryFormat), 1, gRetracer.shaderCacheFile) != 1 || fread(&size, sizeof(size), 1, gRetracer.shaderCacheFile) != 1)
                {
                    gRetracer.reportAndAbort("Failed to read data from cache at %ld as %s: %s", offset, md5_str.c_str(), strerror(ferror(gRetracer.shaderCacheFile)));
                }
                if (binaryFormat == GL_NONE || size == 0)
                {
                    gRetracer.reportAndAbort("Invalid cache metadata at %ld for %s", offset, md5_str.c_str());
                }

                gRetracer.shaderCache[md5_str].format = binaryFormat;
                gRetracer.shaderCache[md5_str].buffer.resize(size);
                if (fread(gRetracer.shaderCache[md5_str].buffer.data(), gRetracer.shaderCache[md5_str].buffer.size(), 1, gRetracer.shaderCacheFile) != 1)
                {
                    gRetracer.reportAndAbort("Failed to read %d bytes of data from cache as %s: %s", size, md5_str.c_str(), strerror(ferror(gRetracer.shaderCacheFile)));
                }
            }
            fclose(idx);
            DBG_LOG("Found shader cache index file, loaded %d cache entries\n", (int)size);
        }
}

bool load_from_shadercache(GLuint program, GLuint originalProgramName, int status)
{
    assert(gRetracer.mOptions.mShaderCacheLoad);

    // check this particular shader
    std::vector<std::string> shaders;
    for (const GLuint shader_id : gRetracer.getCurrentContext().getShaderIDs(program))
    {
        shaders.push_back(gRetracer.getCurrentContext().getShaderSource(shader_id));
    }

    MD5Digest cached_md5(shaders);
    const std::string md5 = cached_md5.text();
    if (gRetracer.shaderCacheIndex.count(md5) == 0)
    {
        gRetracer.reportAndAbort("Could not find shader %s in cache!", md5.c_str());
    }

    if (gRetracer.shaderCacheIndex[md5] == UINT64_MAX)
    {
        if (gRetracer.mOptions.mDebug)
        {
            DBG_LOG("warning: skip load_from_shadercache for program %u because of error linking: status %d\n", originalProgramName, status);
        }
        return false;
    }
    _glGetError(); // clear
    _glProgramBinary(program, gRetracer.shaderCache[md5].format, gRetracer.shaderCache[md5].buffer.data(), gRetracer.shaderCache[md5].buffer.size());
    GLenum err = _glGetError();
    if (err != GL_NO_ERROR)
    {
        gRetracer.reportAndAbort("Failed to upload shader %s from cache for program %u(retraceProgram %u)!", md5.c_str(), originalProgramName, program);
    }
    if (gRetracer.mOptions.mDebug)
    {
        DBG_LOG("Loaded program %u from cache as %s.\n", originalProgramName, md5.c_str());
    }
    return true;
}

static void save_shadercache(GLuint program, GLuint originalProgramName, bool bSkipShadercache)
{
    std::vector<std::string> shaders;
    for (const GLuint shader_id : gRetracer.getCurrentContext().getShaderIDs(program))
    {
        shaders.push_back(gRetracer.getCurrentContext().getShaderSource(shader_id));
    }
    MD5Digest cached_md5(shaders);
    if (gRetracer.shaderCacheIndex.count(cached_md5.text()) == 0)
    {
        if (!bSkipShadercache)
        {
            // save and write binary to disk
            GLint len = 0;
            _glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &len);

            std::vector<char> buffer(len);
            GLenum binaryFormat = GL_NONE;
            _glGetProgramBinary(program, len, NULL, &binaryFormat, (void*)buffer.data());

            const std::string bpath = gRetracer.mOptions.mShaderCacheFile + ".bin";
            FILE* fp = fopen(bpath.c_str(), "ab+");
            if (!fp) gRetracer.reportAndAbort("Failed to open shader cache file %s for writing: %s", bpath.c_str(), strerror(errno));
            uint32_t size = len;
            (void)fseek(fp, 0, SEEK_END);
            long offset = ftell(fp);
            if (fwrite(&binaryFormat, sizeof(GLenum), 1, fp) != 1 || fwrite(&size, sizeof(size), 1, fp) != 1 || fwrite(buffer.data(), buffer.size(), 1, fp) != 1)
            {
                gRetracer.reportAndAbort("Failed to write data to shader cache file: %s", strerror(ferror(gRetracer.shaderCacheFile)));
            }
            fclose(fp);

            gRetracer.shaderCacheIndex[cached_md5.text()] = offset;
            if (gRetracer.mOptions.mDebug)
            {
                DBG_LOG("Saving program %u(retraceProgram %u) to shader cache as %s{.idx|.bin} with offset=%ld size=%ld md5=%s\n", originalProgramName, program, gRetracer.mOptions.mShaderCacheFile.c_str(), offset, (long)size, cached_md5.text().c_str());
            }
        }
        else
        {
            gRetracer.shaderCacheIndex[cached_md5.text()] = UINT64_MAX;
        }
        // Overwrite index on disk
        const std::string ipath = gRetracer.mOptions.mShaderCacheFile + ".idx";
        FILE *fp = fopen(ipath.c_str(), "wb");
        if (!fp)
        {
            gRetracer.reportAndAbort("Failed to open index file %s for writing: %s", ipath.c_str(), strerror(errno));
        }
        if (fwrite(gRetracer.shaderCacheVersionMD5.c_str(), gRetracer.shaderCacheVersionMD5.size(), 1, fp ) != 1)
        {
            gRetracer.reportAndAbort("Failed to write ddk version MD5 (%s) to shader cache index: %s", gRetracer.shaderCacheVersionMD5.c_str(), strerror(ferror(fp)));
        }

        uint32_t entries = gRetracer.shaderCacheIndex.size();
        if (fwrite(&entries, sizeof(entries), 1, fp) != 1)
        {
            gRetracer.reportAndAbort("Failed to write size of data to shader cache index: %s", strerror(ferror(fp)));
        }
        for (const auto &pair : gRetracer.shaderCacheIndex)
        {
            if (fwrite(pair.first.c_str(), pair.first.size(), 1, fp) != 1 || fwrite(&pair.second, sizeof(pair.second), 1, fp) != 1)
            {
                gRetracer.reportAndAbort("Failed to write data to shader cache index: %s", strerror(ferror(fp)));
            }
        }
        fclose(fp);
    }
}

void post_glLinkProgram(GLuint program, GLuint originalProgramName, int status)
{
    bool bSkipShadercache = false;
    GLint linkStatus;
    _glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_TRUE && status == 0)
    {
        // A bit of an odd case: Shader failed during tracing, but succeeds during replay. This can absolutely happen without being an
        // error (eg because feature checks can resolve differently on different platforms), but interesting information when debugging.
        if (gRetracer.mOptions.mDebug) DBG_LOG("Linking program id %u (retrace id %u) failed in trace but works on replay. This is not a problem.\n", originalProgramName, program);
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
            _glGetProgramInfoLog(program, infoLogLength, &len, infoLog);
        }
        vector<unsigned int>::iterator result = find(gRetracer.mOptions.mLinkErrorWhiteListCallNum.begin(), gRetracer.mOptions.mLinkErrorWhiteListCallNum.end(), gRetracer.GetCurCallId());
        if(result != gRetracer.mOptions.mLinkErrorWhiteListCallNum.end())
        {
            DBG_LOG("Error in linking program %u: %s. But this call has already been added to whitelist. So ignore this error and continue retracing, skip shadercache.\n", originalProgramName, infoLog ? infoLog : "(n/a)");
            bSkipShadercache = true;
        }
        else
        {
            gRetracer.reportAndAbort("Error in linking program %u: %s", originalProgramName, infoLog ? infoLog : "(n/a)");
        }
        free(infoLog);
    }
    else if (linkStatus == GL_FALSE && status == 0)
    {
        DBG_LOG("Error in linking program %u(retraceProg %u) both in trace and retrace, skip shadercache.\n", originalProgramName, program);
        bSkipShadercache = true;
    }

    if (gRetracer.mOptions.mShaderCacheFile.size() > 0 && !gRetracer.mOptions.mShaderCacheLoad)
    {
        save_shadercache(program, originalProgramName, bSkipShadercache);
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
            gRetracer.reportAndAbort("Failed to modify shader: %s", shaderMod.getErrorString().c_str());
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
        gRetracer.mpOffscrMgr->BindOffscreenFBO(target);
    }
    else
    {
        glBindFramebuffer(target, framebuffer);
    }

    if (gRetracer.mOptions.mForceVRS != -1)
    {
        _glShadingRateEXT(gRetracer.mOptions.mForceVRS);
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
            gRetracer.mpOffscrMgr->BindOffscreenFBO(GL_FRAMEBUFFER);
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
