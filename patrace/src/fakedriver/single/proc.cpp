#include "../common.h"
#include <sys/types.h>
#include <unistd.h>
#include "EGL/egl.h"

#ifndef ANDROID
#include <cstdlib>
#endif

namespace wrapper
{
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
            const char *strDestDll = sDoIntercept ? sInterceptorPath.c_str() : findFirst(single_driver_search_paths);
#else
            const char *tmpDll = getenv("INTERCEPTOR_LIB");
            const char *strDestDll = tmpDll != NULL ? tmpDll : "libegltrace.so";
#endif
            sInterceptorHandler = dlopen(strDestDll, RTLD_NOW);
            if (sInterceptorHandler == 0) {
                DBG_LOG("Fail to load monolithic library %s. Error msg: %s\n", strDestDll, dlerror());
            } else {
                DBG_LOG("Successfully loaded monolithic library %s\n", strDestDll);
            }
        }
        void *retval = dlsym(sInterceptorHandler, procName);
        if (!retval)
        {
            retval = (void *)eglGetProcAddress(procName);
        }
        return retval;
    }
}
