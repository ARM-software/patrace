#ifndef _MEMORYINFO_HPP_
#define _MEMORYINFO_HPP_
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <unistd.h>
#include <limits>
#include <stdio.h>
#include <sstream>
#include <common/os.hpp>

#ifdef ANDROID
#include <sys/sysinfo.h>
#endif

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_host.h>
#endif

typedef struct
{
    unsigned long size,resident,share,text,lib,data,dt;
} statm_t;

class MemoryInfo
{
    private:
        static unsigned long mMargin; // Memory margin, will be subtracted from available device memory

        static void readMemStat(statm_t& result);

    public:
        static void reserveAndReleaseMemory(unsigned long reserve_mem);
        static void setMarginInBytes(long margin);
        static unsigned long getFreeMemory();
        static unsigned long getFreeMemoryRaw();
};

#endif
