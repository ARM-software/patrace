#include "../common.h"
#include <sys/types.h>
#include <unistd.h>

#ifndef ANDROID
#include <cstdlib>
#endif

namespace wrapper {

    void* CWrapper::GetProcAddress(const char* procName)
    {
        static void *sInterceptorHandler = 0;
        pid_t myPid = getpid();
        static pid_t previousPid = 0;

        if (sInterceptorHandler == 0 || previousPid != myPid)
        {
            previousPid = myPid;
#ifdef ANDROID
            LoadConfigFiles();

            const char *strDestDll = sDoIntercept ?
                sInterceptorPath.c_str() :
                "/system/lib/egl/libOpenCL_mali.so";
#else
            const char *tmpDll = getenv("INTERCEPTOR_LIB");
            const char *strDestDll = tmpDll != NULL ? tmpDll : "libegltrace.so";
#endif

            sInterceptorHandler = dlopen(strDestDll, RTLD_NOW);
            if (sInterceptorHandler == 0)
                DBG_LOG("Fail to load CL driver %s \n", strDestDll);
            else
                DBG_LOG("Successfully loaded CL driver %s \n", strDestDll);
        }

        return dlsym(sInterceptorHandler, procName);
    }

}
