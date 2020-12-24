#ifndef _MEMORYINFO_HPP_
#define _MEMORYINFO_HPP_

class MemoryInfo
{
    public:
        static void reserveAndReleaseMemory(unsigned long reserve_mem);
        static unsigned long getFreeMemory();
        static unsigned long getFreeMemoryRaw();
};

#endif
