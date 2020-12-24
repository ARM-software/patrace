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

PerfCollector::PerfCollector(const Json::Value& config, const std::string& name) : Collector(config, name)
{
    mSet = mConfig.get("set", 0).asInt();
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

enum perf_event_exclude
{
	PERF_EVENT_EXCLUDE_NONE = 0,
	PERF_EVENT_EXCLUDE_KERNEL = 1,
	PERF_EVENT_EXCLUDE_USER = 2
};

static int add_event(int type, int config, int group, int inherit = 1,
                     perf_event_exclude exclude = PERF_EVENT_EXCLUDE_NONE)
{
    struct perf_event_attr pe;

    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = type;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = config;
    pe.disabled = 1;
    pe.inherit = inherit;
    pe.exclude_user = (exclude == PERF_EVENT_EXCLUDE_USER ? 1 : 0) ;
    pe.exclude_kernel = (exclude == PERF_EVENT_EXCLUDE_KERNEL ? 1 : 0);
    pe.exclude_hv = 0;
    const int fd = perf_event_open(&pe, 0, -1, group, 0);
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
    const int group = mCounters["CPUCycleCount"] = add_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, -1);
    if (mSet == 1)
    {
        DBG_LOG("Using CPU counter set number 1, this will fail on non-ARM CPU's\n");
        mCounters["CPUInstructionRetired"] = add_event(PERF_TYPE_RAW, 0x8, group);
        mCounters["CPUL1CacheAccesses"] = add_event(PERF_TYPE_RAW, 0x4, group);
        mCounters["CPUL2CacheAccesses"] = add_event(PERF_TYPE_RAW, 0x16, group);
        mCounters["CPULASESpec"] = add_event(PERF_TYPE_RAW, 0x74, group); // simd instruction
        mCounters["CPUVFPSpec"] = add_event(PERF_TYPE_RAW, 0x75, group); // float instruction
        mCounters["CPUCryptoSpec"] = add_event(PERF_TYPE_RAW, 0x77, group);
    }
    else if (mSet == 2)
    {
        DBG_LOG("Using CPU counter set number 2, this will fail on non-ARM CPU's\n");
        mCounters["CPUL3CacheAccesses"] = add_event(PERF_TYPE_RAW, 0x2b, group);
        mCounters["CPUBusAccessRead"] = add_event(PERF_TYPE_RAW, 0x60, group);
        mCounters["CPUBusAccessWrite"] = add_event(PERF_TYPE_RAW, 0x61, group);
        mCounters["CPUMemoryAccessRead"] = add_event(PERF_TYPE_RAW, 0x66, group);
        mCounters["CPUMemoryAccessWrite"] = add_event(PERF_TYPE_RAW, 0x67, group);
    }
    else if (mSet == 3)
    {
        DBG_LOG("Using CPU counter set number 3, this will fail on non-ARM CPU's\n");
        mCounters["CPUCycles"] = add_event(PERF_TYPE_RAW, 0x11, group);
        mCounters["CPUBusAccesses"] = add_event(PERF_TYPE_RAW, 0x19, group);
        mCounters["CPUL2CacheRead"] = add_event(PERF_TYPE_RAW, 0x050, group);
        mCounters["CPUL2CacheWrite"] = add_event(PERF_TYPE_RAW, 0x51, group);
        mCounters["CPUMemoryAccessRead"] = add_event(PERF_TYPE_RAW, 0x66, group);
        mCounters["CPUMemoryAccessWrite"] = add_event(PERF_TYPE_RAW, 0x67, group);
    }
    else if (mSet == 4)
    {
        DBG_LOG("Using CPU counter set number 4, this will fail on non-ARM CPU's\n");
        /* All Threads */
        mCounters["CPUCyclesUser"] = add_event(PERF_TYPE_RAW, 0x11, group, 1, PERF_EVENT_EXCLUDE_KERNEL);
        mCounters["CPUCyclesKernel"] = add_event(PERF_TYPE_RAW, 0x11, group, 1, PERF_EVENT_EXCLUDE_USER);

        /* Main Thread */
        const int group_main_thread = mCounters["CPUCycleCountMainThread"] =
            add_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, -1, 0);
        mCounters["CPUCyclesUserMainThread"] =
            add_event(PERF_TYPE_RAW, 0x11, group_main_thread, 0, PERF_EVENT_EXCLUDE_KERNEL);
        mCounters["CPUCyclesKernelMainThread"] =
            add_event(PERF_TYPE_RAW, 0x11, group_main_thread, 0, PERF_EVENT_EXCLUDE_USER);
    }
    else // default set, same as for x86
    {
        DBG_LOG("Using CPU counter set number 0 (default)\n");
        mCounters["CPUInstructionCount"] = add_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, group);
        mCounters["CPUCacheReferences"] = add_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES, group);
        mCounters["CPUCacheMisses"] = add_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES, group);
        mCounters["CPUBranchMispredictions"] = add_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES, group);
    }
    for (const auto pair : mCounters) if (pair.second == -1) { DBG_LOG("libcollector perf: Failed to init counter %s\n", pair.first.c_str()); return false; }
    return true;
}

bool PerfCollector::deinit()
{
    for (const auto pair : mCounters)
        if (pair.second != -1)
            close(pair.second);
    mCounters.clear();
    return true;
}

bool PerfCollector::start()
{
    if (mCollecting)
        return true;

    for (const auto pair : mCounters)
    {
        if (ioctl(pair.second, PERF_EVENT_IOC_RESET, 0) == -1)
        {
            perror("ioctl PERF_EVENT_IOC_RESET");
            return false;
        }

        if (ioctl(pair.second, PERF_EVENT_IOC_ENABLE, 0) == -1)
        {
            perror("ioctl PERF_EVENT_IOC_ENABLE");
            return false;
        }
    }

    mCollecting = true;

    return true;
}

bool PerfCollector::stop()
{
    if (!mCollecting)
        return true;

    DBG_LOG("Stopping perf collection.\n");
    for (const auto pair : mCounters)
        if (pair.second != -1)
        {
            if (ioctl(pair.second, PERF_EVENT_IOC_DISABLE, 0) == -1)
            {
                perror("ioctl PERF_EVENT_IOC_DISABLE");
                return false;
            }
        }

    mCollecting = false;

    return true;
}

bool PerfCollector::collect(int64_t /* now */)
{
    long long count = 0;

    if (!mCollecting)
        return false;

    for (const auto pair : mCounters)
    {
        if (pair.second == -1) continue;
        if (read(pair.second, &count, sizeof(long long)) == -1)
        {
            perror("read");
            return false;
        }

        if (ioctl(pair.second, PERF_EVENT_IOC_RESET, 0) == -1)
        {
            perror("ioctl PERF_EVENT_IOC_RESET");
            return false;
        }
        add(pair.first, count);
    }

    return true;
}
