#include <common/memoryinfo.hpp>

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

void MemoryInfo::reserveAndReleaseMemory(unsigned long reserve_mem)
{
    long before = getFreeMemoryRaw();
    void *ptr = malloc(1024);
    if (ptr == NULL){
        DBG_LOG("Memory allocation FAILED\n");
        return;
    }
    int cycles = 20;
    for (int i = 1; i < cycles; ++i){
        long mem_size = reserve_mem*(double(i)/cycles);
        void* ptr2 = realloc(ptr, mem_size);
        if (ptr2 != NULL){
            ptr = ptr2;
            DBG_LOG("Trying to reallocate: %ld MiB\n", mem_size / (1024 *1024));
            memset(ptr, 0, mem_size);
            DBG_LOG("Memory reallocation successfull: %ld MiB\n", mem_size / (1024 *1024));
            DBG_LOG("Free mem: %ld\n", getFreeMemoryRaw() / (1024 *1024));
            //reserve_mem = getFreeMemoryRaw();
        }
        else {
            free(ptr);
            ptr = NULL;
            DBG_LOG("Memory reallocation FAILED\n");
        }
    }
    free(ptr);
    sleep(5); // Sleep, apps may be in process of getting killed or respawning
    long after = getFreeMemoryRaw();
    DBG_LOG("Claimed memory: %ld\n", (after-before)/(1024*1024));
}

unsigned long MemoryInfo::getFreeMemory()
{
    unsigned long free_mem = getFreeMemoryRaw();
    return free_mem;
}

/* Returns free memory in bytes */
unsigned long MemoryInfo::getFreeMemoryRaw()
{
    unsigned long free_mem = 0;
#if defined(ANDROID) || defined(__linux__)
        std::string token;
        std::ifstream file("/proc/meminfo");
        while(file >> token) {
            if(token == "MemFree:") {
                unsigned long mem;
                if(file >> mem) {
                    free_mem += mem;
                }
            }
            else if(token == "Buffers:") {
                unsigned long mem;
                if(file >> mem) {
                    free_mem += mem;
                }
            }
            else if(token == "Cached:") {
                unsigned long mem;
                if(file >> mem) {
                    free_mem += mem;
                }
            }
            // ignore rest of the line
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        free_mem = free_mem * 1024; // To bytes
#elif __APPLE__
        mach_port_t host_port;
        mach_msg_type_number_t host_size;
        vm_size_t pagesize;

        host_port = mach_host_self();
        host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
        host_page_size(host_port, &pagesize);

        vm_statistics_data_t vm_stat;

        if (host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) != KERN_SUCCESS)
        {
            DBG_LOG("Failed to fetch vm info\n");
        }

        natural_t mem_free = (vm_stat.free_count + vm_stat.inactive_count) * pagesize;

        free_mem = mem_free;
#endif
    return free_mem;
}
