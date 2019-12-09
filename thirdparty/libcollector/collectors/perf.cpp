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

bool PerfCollector::init()
{
    struct perf_event_attr pe;

    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_CPU_CYCLES;
    pe.disabled = 1;
    pe.inherit = 1;

    /* NOTE: exclude_kernel enables including kernel time in the measurement, which is probably what we want.
     * If you set this to 1, you may get "Permission denied" on the syscall on some systems (Odroid-Q).
     */
    pe.exclude_kernel = 0;
    pe.exclude_hv = 0;

    mPerfFD = perf_event_open(&pe, 0, -1, -1, 0);
    if (mPerfFD < 0)
    {
        DBG_LOG("Error opening perf: error %d\n", errno);
        perror("syscall");
        mPerfFD = -1;
        return false;
    }

    return true;
}

bool PerfCollector::deinit()
{
    if (mPerfFD == -1)
        return true;

    if (close(mPerfFD) == -1)
    {
        perror("close");
        return false;
    }
    mPerfFD = -1;

    return true;
}

bool PerfCollector::start()
{
    if (mCollecting)
        return true;

    if (mPerfFD == -1)
        return false;

    if (ioctl(mPerfFD, PERF_EVENT_IOC_RESET, 0) == -1)
    {
        perror("ioctl PERF_EVENT_IOC_RESET");
        return false;
    }

    if (ioctl(mPerfFD, PERF_EVENT_IOC_ENABLE, 0) == -1)
    {
        perror("ioctl PERF_EVENT_IOC_ENABLE");
        return false;
    }

    mCollecting = true;

    return true;
}

bool PerfCollector::stop()
{
    if (!mCollecting)
        return true;

    if (mPerfFD != -1)
    {
        DBG_LOG("Stopping perf collection.\n");
        if (ioctl(mPerfFD, PERF_EVENT_IOC_DISABLE, 0) == -1)
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

    if (mPerfFD == -1)
        return false;

    if (read(mPerfFD, &count, sizeof(long long)) == -1)
    {
        perror("read");
        return false;
    }

    if (ioctl(mPerfFD, PERF_EVENT_IOC_RESET, 0) == -1)
    {
        perror("ioctl PERF_EVENT_IOC_RESET");
        return false;
    }

    add("CPUCycleCount", count);

    return true;
}
