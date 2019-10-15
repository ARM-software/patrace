#include "common.h"

#ifdef ANDROID
#include <android/log.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>

#include <fstream>

using namespace std;

#ifdef __LP64__
    #define SYSTEM_VENDOR_LIB_PREFIX "/system/vendor/lib64/egl/"
    #define SYSTEM_LIB_PREFIX "/system/lib64/egl/"
#else
    #define SYSTEM_VENDOR_LIB_PREFIX "/system/vendor/lib/egl/"
    #define SYSTEM_LIB_PREFIX "/system/lib/egl/"
#endif

// Fakedriver search paths
const char* gles2_search_paths[] = {
    SYSTEM_VENDOR_LIB_PREFIX "libGLESv2_adreno.so",
    SYSTEM_VENDOR_LIB_PREFIX "wrapped_libGLESv2.so",
    SYSTEM_LIB_PREFIX "libGLESv2_mali.so",
    NULL
};
const char* gles1_search_paths[] = {
    SYSTEM_VENDOR_LIB_PREFIX "libGLESv1_CM_adreno.so",
    SYSTEM_VENDOR_LIB_PREFIX "wrapped_libGLESv1_CM.so",
    SYSTEM_LIB_PREFIX "libGLESv1_CM_mali.so",
    NULL
};
const char* egl_search_paths[] = {
    SYSTEM_VENDOR_LIB_PREFIX "libEGL_adreno.so",
    SYSTEM_VENDOR_LIB_PREFIX "wrapped_libEGL.so",
    SYSTEM_LIB_PREFIX "libEGL_mali.so",
    NULL
};
const char* single_driver_search_paths[] = {
    SYSTEM_VENDOR_LIB_PREFIX "wrapped_libGLES.so",
    SYSTEM_VENDOR_LIB_PREFIX "libGLES_mali.so",
    SYSTEM_LIB_PREFIX "libGLES_mali.so",
    SYSTEM_VENDOR_LIB_PREFIX "lib_mali.so",      // for Android 8
    NULL
};

// Configuration files
const char* applist_cfg_search_paths[] = {
    SYSTEM_VENDOR_LIB_PREFIX "appList.cfg",
    SYSTEM_LIB_PREFIX "appList.cfg",
    NULL,
};
const char* fps_applist_cfg_search_paths[] = {
    SYSTEM_VENDOR_LIB_PREFIX "fpsAppList.cfg",
    SYSTEM_LIB_PREFIX "fpsAppList.cfg",
    NULL,
};
const char* interceptor_cfg_search_paths[] = {
    SYSTEM_VENDOR_LIB_PREFIX "interceptor.cfg",
    SYSTEM_LIB_PREFIX "interceptor.cfg",
    NULL,
};

#undef SYSTEM_VENDOR_LIB_PREFIX
#undef SYSTEM_LIB_PREFIX

namespace wrapper
{

    // Initialization of static variables
    bool CWrapper::sDoIntercept = false;
    string CWrapper::sInterceptorPath;

    bool CWrapper::sShowFPS= false;

    void CWrapper::log(const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);
#ifdef ANDROID
#ifdef PLATFORM_64BIT
        __android_log_vprint(ANDROID_LOG_INFO, "fakedriver64", format, ap);
#else
        __android_log_vprint(ANDROID_LOG_INFO, "fakedriver32", format, ap);
#endif
#else
        vprintf(format, ap);
#endif
        va_end(ap);
    }

    void CWrapper::LoadConfigFiles()
    {
// On the linux platform, we don't need to use configuation files.
#ifdef ANDROID
        string procName = getProcessName();

        // 1. Figure out if we want to show FPS in adb logcat for the current process
        ifstream fpsAppListIfs(findFirst(fps_applist_cfg_search_paths));
        if (fpsAppListIfs.is_open())
        {
            string itAppName;
            while (fpsAppListIfs>>itAppName)
            {
                if (itAppName.compare(procName) == 0)
                {
                    sShowFPS = true;
                    break;
                }
            }
        }

        // 2. Figure out if we want to intercept the current process.
        const char* appListPath = findFirst(applist_cfg_search_paths);
        ifstream appListIfs(appListPath);
        if (!appListIfs.is_open())
        {
            DBG_LOG("Failed to open %s\n", appListPath);
            return;
        }

        string itAppName;
        while (appListIfs>>itAppName)
        {
            if (itAppName.compare(procName) == 0)
            {
                sDoIntercept = true;
                break;
            }
        }

        if (sDoIntercept)
        {
            DBG_LOG("Begin intercept %s !\n", procName.c_str());

            // 3. Figure out which module do we forward the function calls to.
            const char* interceptorCfgPath = findFirst(interceptor_cfg_search_paths);
            ifstream interceptorIfs(interceptorCfgPath);
            if (!interceptorIfs.is_open())
            {
                sDoIntercept = false;
                DBG_LOG("Failed to open %s\n", interceptorCfgPath);
                return;
            }
            interceptorIfs>>sInterceptorPath;
        }
        else
            DBG_LOG("Don't intercept %s !\n", procName.c_str());
#endif
    }

    string CWrapper::getProcessName()
    {
        // using the id, find the name of this process
        char currentProcessName[128];
        char path[64];
        sprintf(path, "/proc/%d/cmdline", getpid());
        memset(currentProcessName, 0, sizeof(currentProcessName));
        int pinfo = open(path, O_RDONLY);
        if (pinfo)
        {
            // Read process name from file descriptor pinfo
            // http://linux.die.net/man/2/read
            size_t r = read(pinfo, currentProcessName, 64);
            if (r <= 0)
            {
                DBG_LOG("Could not read process name.\n");
            }
            DBG_LOG("Current process: %s (%d)\n", currentProcessName, getpid());
            close(pinfo);
        }
        else
        {
            DBG_LOG("Failed to open: %s\n", path);
        }
        string str(currentProcessName);
        return str;
    }

    const char* findFirst(const char** paths)
    {
        const char* i;
        const char* last = "NONE";
        while ((i = *paths++))
        {
            if (access(i, R_OK) == 0)
            {
                 return i;
            }
            last = i;
        }
        return last; // for informative error message and avoid crashing
    }

}
