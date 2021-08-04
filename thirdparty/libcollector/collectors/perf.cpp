#include "perf.hpp"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#if !defined(ANDROID)
#include <linux/perf_event.h>
#else
#include "perf_event.h"
#endif
#include <asm/unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <sstream>
#include <fstream>

std::map<int, std::vector<struct event>> EVENTS = {
{0, { {"CPUInstructionRetired", PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, false, false, hw_cnt_length::b32},
      {"CPUCacheReferences", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES, false, false, hw_cnt_length::b32},
      {"CPUCacheMisses", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES, false, false, hw_cnt_length::b32},
      {"CPUBranchMispredictions", PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES, false, false, hw_cnt_length::b32}
    }
},
{1, { {"CPUInstructionRetired", PERF_TYPE_RAW, 0x8, false, false, hw_cnt_length::b32},
      {"CPUL1CacheAccesses", PERF_TYPE_RAW, 0x4, false, false, hw_cnt_length::b32},
      {"CPUL2CacheAccesses", PERF_TYPE_RAW, 0x16, false, false, hw_cnt_length::b32},
      {"CPULASESpec", PERF_TYPE_RAW, 0x74, false, false, hw_cnt_length::b32},
      {"CPUVFPSpec", PERF_TYPE_RAW, 0x75, false, false, hw_cnt_length::b32},
      {"CPUCryptoSpec", PERF_TYPE_RAW, 0x77, false, false, hw_cnt_length::b32},
    }
},
{2, { {"CPUL3CacheAccesses", PERF_TYPE_RAW, 0x2b, false, false, hw_cnt_length::b32},
      {"CPUBusAccessRead", PERF_TYPE_RAW, 0x60, false, false, hw_cnt_length::b32},
      {"CPUBusAccessWrite", PERF_TYPE_RAW, 0x61, false, false, hw_cnt_length::b32},
      {"CPUMemoryAccessRead", PERF_TYPE_RAW, 0x66, false, false, hw_cnt_length::b32},
      {"CPUMemoryAccessWrite", PERF_TYPE_RAW, 0x67, false, false, hw_cnt_length::b32},
    }
},
{3, { {"CPUBusAccesses", PERF_TYPE_RAW, 0x19, false, false, hw_cnt_length::b32},
      {"CPUL2CacheRead", PERF_TYPE_RAW, 0x50, false, false, hw_cnt_length::b32},
      {"CPUL2CacheWrite", PERF_TYPE_RAW, 0x51, false, false, hw_cnt_length::b32},
      {"CPUMemoryAccessRead", PERF_TYPE_RAW, 0x66, false, false, hw_cnt_length::b32},
      {"CPUMemoryAccessWrite", PERF_TYPE_RAW, 0x67, false, false, hw_cnt_length::b32},
    }
}
};

PerfCollector::PerfCollector(const Json::Value& config, const std::string& name) : Collector(config, name)
{
    mSet = mConfig.get("set", -1).asInt();

    if ((0 <= mSet) && (mSet <= 3))
    {
        DBG_LOG("Using reserved CPU counter set number %d, this will fail on non-ARM CPU's except set 0.\n", mSet);
        for (struct event& e : EVENTS[mSet])
            mEvents.push_back(e);
    }
    else if (mConfig.isMember("event"))
    {
        DBG_LOG("Using customized CPU counter event, this will fail on non-ARM CPU's\n");
        Json::Value eventArray = mConfig["event"];
        for (Json::ArrayIndex i = 0; i < eventArray.size(); i++)
        {
            Json::Value item = eventArray[i];
            struct event e;

            if ( !item.isMember("name") || !item.isMember("type") || !item.isMember("config") )
            {
                DBG_LOG("perf event does not specify name, tpye or config, skip this event!\n");
                continue;
            }
            e.name = item.get("name", "").asString();
            e.type = item.get("type", -1).asInt();
            e.config = item.get("config", -1).asInt();
            e.exc_user = item.get("excludeUser", false).asBool();
            e.exc_kernel = item.get("excludeKernel", false).asBool();
            e.len = (item.get("counterLen64bit", 0).asInt() == 0) ? hw_cnt_length::b32 : hw_cnt_length::b64;

            mEvents.push_back(e);
        }
    }

    mAllThread = mConfig.get("allthread", true).asBool();
}

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags)
{
    int ret;

    hw_event->size = sizeof(*hw_event);
    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);

    return ret;
}

bool PerfCollector::available()
{
    return true;
}

static int add_event(int type, int config, int group, int tid, bool exclude_user = false,
                        bool exclude_kernel = false, enum hw_cnt_length len = hw_cnt_length::b32)
{
    struct perf_event_attr pe;

    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = type;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = config;
    pe.config1 = (len == hw_cnt_length::b32) ? 0 : 1;
    pe.disabled = 1;
    pe.inherit = 1;
    pe.exclude_user = exclude_user;
    pe.exclude_kernel = exclude_kernel;
    pe.exclude_hv = 0;
    pe.read_format = PERF_FORMAT_GROUP;

    const int fd = perf_event_open(&pe, tid, -1, group, 0);
    if (fd < 0)
    {
        DBG_LOG("Error opening perf: error %d\n", errno);
        perror("syscall");
        return -1;
    }
    return fd;
}

bool PerfCollector::init()
{
    if (mEvents.size() == 0)
    {
        DBG_LOG("None perf event counter.\n");
        return false;
    }

    create_perf_thread();

    for (perf_thread& t : mReplayThreads)
        t.eventCtx.init(t.tid, mEvents);

    for (perf_thread& t : mBgThreads)
        t.eventCtx.init(t.tid, mEvents);

    return true;
}

bool PerfCollector::deinit()
{
    for (perf_thread& t : mReplayThreads)
       t.eventCtx.deinit();

    for (perf_thread& t : mBgThreads)
       t.eventCtx.deinit();

    mEvents.clear();
    mReplayThreads.clear();
    mBgThreads.clear();

    clear();

    return true;
}

bool PerfCollector::start()
{
    if (mCollecting)
        return true;

    for (perf_thread& t : mReplayThreads)
        if ( !t.eventCtx.start() )
            return false;

    for (perf_thread& t : mBgThreads)
        if ( !t.eventCtx.start() )
            return false;

    mCollecting = true;
    return true;
}

void PerfCollector::clear()
{
    if (mCollecting)
        return;

    for (perf_thread& t: mReplayThreads)
        t.clear();

    for (perf_thread& t: mBgThreads)
        t.clear();

    Collector::clear();
}

bool PerfCollector::stop()
{
    if (!mCollecting)
        return true;

    DBG_LOG("Stopping perf collection.\n");

    for (perf_thread& t : mReplayThreads)
    {
       t.eventCtx.stop();
    }

    for (perf_thread& t : mBgThreads)
    {
       t.eventCtx.stop();
    }

    mCollecting = false;

    return true;
}

bool PerfCollector::collect(int64_t now)
{
    if (!mCollecting)
        return false;

    struct snapshot snap;
    for (perf_thread& t : mReplayThreads)
    {
        snap = t.eventCtx.collect(now);
        t.update_data(snap);
    }

    for (perf_thread& t : mBgThreads)
    {
        snap = t.eventCtx.collect(now);
        t.update_data(snap);
    }

    return true;
}

bool PerfCollector::postprocess(const std::vector<int64_t>& timing)
{
    Json::Value v;

    if (isSummarized()) mCustomResult["summarized"] = true;
    mCustomResult["thread_data"] = Json::arrayValue;

    Json::Value replayValue;
    replayValue["CCthread"] = "replayMainThreads";

    for (perf_thread& t : mReplayThreads)
    {
        t.postprocess(replayValue);
    }
    mCustomResult["thread_data"].append(replayValue);

    if (mAllThread)
    {
        Json::Value bgValue;
        bgValue["CCthread"] = "backgroundThreads";

        Json::Value allValue(replayValue);
        allValue["CCthread"] = "allThreads";

        for (perf_thread& t : mBgThreads)
        {
            t.postprocess(bgValue);
            t.postprocess(allValue);
        }
        mCustomResult["thread_data"].append(bgValue);
        mCustomResult["thread_data"].append(allValue);
    }

    return true;
}

void PerfCollector::summarize()
{
    mIsSummarized = true;

    for (perf_thread& t : mReplayThreads)
    {
        t.summarize();
    }

    for (perf_thread& t : mBgThreads)
    {
        t.summarize();
    }
}

bool event_context::init(int tid, std::vector<struct event> &events)
{
    struct counter grp;

    grp.fd = group = add_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, -1, tid);
    grp.name = "CPUCycleCount";
    mCounters.push_back(grp);

    for (const struct event& e : events)
    {
        struct counter c;
        c.fd = add_event(e.type, e.config, group, tid, e.exc_user, e.exc_kernel, e.len);
        c.name = e.name;
        mCounters.push_back(c);
    }

    ioctl(group, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(group, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);

    for (const struct counter& c : mCounters) if (c.fd == -1) { DBG_LOG("libcollector perf: Failed to init counter %s\n", c.name.c_str()); return false; }
    return true;
}

bool event_context::deinit()
{
    for (const struct counter& c : mCounters)
        if (c.fd != -1)
            close(c.fd);

    mCounters.clear();
    return true;
}

bool event_context::start()
{
    if (ioctl(group, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) == -1)
    {
        perror("ioctl PERF_EVENT_IOC_RESET");
        return false;
    }
    if (ioctl(group, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) == -1)
    {
        perror("ioctl PERF_EVENT_IOC_ENABLE");
        return false;
    }
    return true;
}

bool event_context::stop()
{
    if (ioctl(group, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP) == -1)
    {
        perror("ioctl PERF_EVENT_IOC_DISABLE");
        return false;
    }

    return true;
}

struct snapshot event_context::collect(int64_t now)
{
    struct snapshot snap;

    if (read(group, &snap, sizeof(snap)) == -1)    perror("read");
    if (ioctl(group, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) == -1)    perror("ioctl PERF_EVENT_IOC_RESET");
    return snap;
}

static std::string getThreadName(int tid)
{
    std::stringstream comm_path;
    if (tid == 0)
        comm_path << "/proc/self/comm";
    else
        comm_path << "/proc/self/task/" << tid << "/comm";

    std::string name;
    std::ifstream comm_file { comm_path.str() };
    if (!comm_file.is_open())
    {
        DBG_LOG("Fail to open comm file for thread %d.\n", tid);
    }
    comm_file >> name;
    return name;
}

void PerfCollector::create_perf_thread()
{
    std::string current_pName = getThreadName(0);

    DIR *dirp = NULL;
    if ( (dirp = opendir("/proc/self/task")) == NULL)
        return;

    struct dirent *ent = NULL;
    while ( (ent = readdir(dirp)) != NULL)
    {
        if ( isdigit(ent->d_name[0]) )
        {
            int tid =_stol( std::string(ent->d_name) );
            std::string thread_name = getThreadName(tid);

#ifdef ANDROID
            if (!strncmp(thread_name.c_str(), "GLThread", 9) || !strncmp(thread_name.c_str(), "Thread-", 7))
                mReplayThreads.emplace_back(tid, thread_name);
#else
            if ( !strcmp(thread_name.c_str(), current_pName.c_str()) )        mReplayThreads.emplace_back(tid, thread_name);
#endif
            if ( mAllThread && !strncmp(thread_name.c_str(), "mali-", 5) )    mBgThreads.emplace_back(tid, thread_name);
        }
    }
    closedir(dirp);
}

static void writeCSV(int tid, std::string name, CollectorValueResults &results)
{
    std::stringstream ss;
#ifdef ANDROID
    ss << "/sdcard/" << name.c_str() << tid << ".csv";
#else
    ss << name.c_str() << tid << ".csv";
#endif
    std::string filename = ss.str();
    DBG_LOG("writing perf result to %s\n", filename.c_str());

    FILE *fp = fopen(filename.c_str(), "w");
    if (fp)
    {
        unsigned int number = 0;
        std::string item;
        for (const auto pair : results)
        {
            item += pair.first + ",";
            number = pair.second.size();
        }
        fprintf(fp, "%s\n", item.c_str());

        for (unsigned int i=0; i<number; i++)
        {
            std::stringstream css;
            std::string value;
            for (const auto pair : results)
            {
                css << pair.second.at(i).i64 << ",";
            }
            value = css.str();
            fprintf(fp, "%s\n", value.c_str());
        }

        fsync(fileno(fp));
        fclose(fp);
    }
    else DBG_LOG("Fail to open %s\n", filename.c_str());
}

void PerfCollector::saveResultsFile()
{
    for (perf_thread& t : mReplayThreads)
        writeCSV(t.tid, t.name, t.mResultsPerThread);

    for (perf_thread& t : mBgThreads)
        writeCSV(t.tid, t.name, t.mResultsPerThread);
}
