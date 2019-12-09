#include "rusage.hpp"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

bool RusageCollector::init()
{
    memset(&prev, 0, sizeof(prev));
    return true;
}

bool RusageCollector::deinit()
{
    return true;
}

bool RusageCollector::available()
{
    return true;
}

bool RusageCollector::start()
{
    if (mCollecting)
    {
        return true;
    }
    if (getrusage(RUSAGE_SELF, &prev) != 0)
    {
        DBG_LOG("Failed to get initial rusage statistics: %s\n", strerror(errno));
        return false;
    }
    mCollecting = true;
    DBG_LOG("Starting rusage per-frame collection\n");
    return true;
}

bool RusageCollector::collect(int64_t /* now */)
{
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0)
    {
        DBG_LOG("Failed to get usage statistics: %s\n", strerror(errno));
        return false;
    }
    struct timeval userdiff;
    struct timeval kerneldiff;
    timersub(&usage.ru_utime, &prev.ru_utime, &userdiff);
    timersub(&usage.ru_stime, &prev.ru_stime, &kerneldiff);

    add("KernelCPUTime", kerneldiff.tv_sec * 1000 * 1000 + kerneldiff.tv_usec);
    add("UserCPUTime", userdiff.tv_sec * 1000 * 1000 + userdiff.tv_usec);
    add("PageFaultsNoIO", usage.ru_minflt - prev.ru_minflt);
    add("PageFaultsWithIO", usage.ru_majflt - prev.ru_majflt);
    add("IOBlockOnInput", usage.ru_inblock - prev.ru_inblock);
    add("IOBlockOnOutput", usage.ru_oublock - prev.ru_oublock);
    add("VoluntaryContextSwitches", usage.ru_nvcsw - prev.ru_nvcsw);
    add("InvoluntaryContextSwitches", usage.ru_nivcsw - prev.ru_nivcsw);

    prev = usage;
    return true;
}
