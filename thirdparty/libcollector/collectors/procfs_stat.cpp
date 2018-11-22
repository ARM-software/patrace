#include "procfs_stat.hpp"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "collector_utility.hpp"

static std::vector<std::string> getStatPaths()
{
    std::string f = "/proc/" + _to_string(getpid()) + "/stat";
    return { f };
}

ProcFSStatCollector::ProcFSStatCollector(const Json::Value& config, const std::string& name)
    : SysfsCollector(config, name, getStatPaths()),
      mTicks(sysconf(_SC_CLK_TCK)),
      mLastSampleTime(0)
{
    if (!mTicks)
    {
        mTicks = 1; // avoid dbz
    }
}

bool ProcFSStatCollector::parse(const char* buffer)
{
    int pid;
    char comm[256];
    char state;
    int ppid;
    int pgrp;
    int session;
    int tty_nr;
    int tpgid;
    unsigned int flags;
    unsigned long minflt;
    unsigned long cminflt;
    unsigned long majflt;
    unsigned long cmajflt;
    unsigned long utime;
    unsigned long stime;
    unsigned int cutime;
    unsigned int cstime;
    int priority;
    int nice;
    int num_threads;

    int ret = sscanf(buffer, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %u %u %d %d %d",
                     &pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags,
                     &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime, &cutime, &cstime, &priority,
                     &nice, &num_threads);

    if (ret != 20)
    {
        return false;
    }

    unsigned long tot_time = utime + stime + cutime + cstime;

    /* record 0 for the first frame as we don't have comparison point */
    if (mLastSampleTime == 0)
    {
        mLastSampleTime = tot_time;
        add("cpu_time", 0.0f);
    }
    else
    {
        add("cpu_time", double(tot_time - mLastSampleTime) / double(mTicks));
        mLastSampleTime = tot_time;
    }
    add("threads", num_threads);

    return true;
}
