#include "memory.hpp"

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

bool MemoryCollector::init()
{
    initialAvailableRAM = getFreeSystemMemory();
    return true;
}

bool MemoryCollector::available()
{
    return true;
}

bool MemoryCollector::deinit()
{
    return true;
}

bool MemoryCollector::start()
{
    if (mCollecting)
    {
        return true;
    }
    mCollecting = true;
    DBG_LOG("Starting memory per-frame collection (%lu total memory, %lu available, %lu available at init)\n",
            (unsigned long)(getTotalSystemMemory() / 1024), (unsigned long)(getFreeSystemMemory() / 1024),
            (unsigned long)(initialAvailableRAM / 1024));
    return true;
}

bool MemoryCollector::collect(int64_t /* now */)
{
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0)
    {
        DBG_LOG("Failed to get usage statistics: %s\n", strerror(errno));
        return false;
    }
    int64_t availableNow = getFreeSystemMemory();
    int64_t diff = initialAvailableRAM - availableNow;

    add("memory_max_rss", usage.ru_maxrss);
    add("memory_cur_rss", getCurrentRSS() / 1024);
    add("memory_used", diff / 1024);

    return true;
}
