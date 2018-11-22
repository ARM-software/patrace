// Place to put experimental collectors and noisy output

#include "debug.hpp"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>
#endif

bool DebugCollector::init()
{
    return true;
}

bool DebugCollector::available()
{
    return true;
}

bool DebugCollector::deinit()
{
    return true;
}

static size_t getCurrentRSS()
{
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
    return (size_t)info.WorkingSetSize;
#elif defined(__APPLE__) && defined(__MACH__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
                    (task_info_t)&info, &infoCount ) != KERN_SUCCESS )
        return (size_t)0L;      /* Can't access? */
    return (size_t)info.resident_size;
#else
    long rss = 0L;
    FILE* fp = NULL;
    if ( (fp = fopen( "/proc/self/statm", "r" )) == NULL )
        return (size_t)0L;      /* Can't open? */
    if ( fscanf( fp, "%*s%ld", &rss ) != 1 )
    {
        fclose( fp );
        return (size_t)0L;      /* Can't read? */
    }
    fclose( fp );
    return (size_t)rss * (size_t)sysconf( _SC_PAGESIZE);
#endif
}

static size_t getTotalSystemMemory()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

static size_t getFreeSystemMemory()
{
    long pages = sysconf(_SC_AVPHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

bool DebugCollector::start()
{
    if (mCollecting)
    {
        return true;
    }
    mCollecting = true;
    DBG_LOG("Starting debug per-frame collection\n");
    return true;
}

bool DebugCollector::collect(int64_t /* now */)
{
    static int index = 0;
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0)
    {
        DBG_LOG("Failed to get usage statistics: %s\n", strerror(errno));
        return false;
    }
    DBG_LOG("[%d] User %ld.%06ld, kernel %ld.%06ld, memory max %ld curr %ld, vol ctx %ld, invol ctx %ld; free mem %lu / %lu\n",
            index, usage.ru_utime.tv_sec, usage.ru_utime.tv_usec, usage.ru_stime.tv_sec, usage.ru_stime.tv_usec,
            usage.ru_maxrss, (long)(getCurrentRSS() / 1024), usage.ru_nvcsw, usage.ru_nivcsw,
            (unsigned long)(getFreeSystemMemory() / 1024), (unsigned long)(getTotalSystemMemory() / 1024));
    index++;
    return true;
}
