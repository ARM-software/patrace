#include "../common.h"
#include <sys/types.h>
#include <unistd.h>
#include "dispatch/gleslayer_helper.h"

#ifndef ANDROID
#include <cstdlib>
#endif

namespace wrapper
{
    void* CWrapper::GetProcAddress(const char* procName)
    {
#ifdef GLESLAYER
        void *retval = dispatch_intercept_func(PATRACE_LAYER_NAME, procName);
        return retval;
#else
        static void *sInterceptorHandler = 0;
        pid_t myPid = getpid();
        static pid_t previousPid = 0;

        if (sInterceptorHandler == 0 || previousPid != myPid)
        {
            previousPid = myPid;
#ifdef ANDROID
            LoadConfigFiles();
            const char *strDestDll = sDoIntercept ? sInterceptorPath.c_str() : findFirst(gles1_search_paths);
#else
            const char *tmpDll = getenv("INTERCEPTOR_LIB");
            const char *strDestDll = tmpDll != NULL ? tmpDll : "libegltrace.so";
#endif
            sInterceptorHandler = dlopen(strDestDll, RTLD_NOW);
            if (sInterceptorHandler == 0)
                DBG_LOG("Fail to load GLES1 library %s. Error msg: %s \n", strDestDll, dlerror());
            else
                DBG_LOG("Successfully loaded GLES1 library %s \n", strDestDll);
        }
        return dlsym(sInterceptorHandler, procName);
#endif
    }
}
