#ifndef _FAKE_DRIVER_COMMON_H_
#define _FAKE_DRIVER_COMMON_H_

#include <string>
#include <vector>
#include <string.h>
#include <dlfcn.h>

#ifndef GLESLAYER

// Fakedriver search paths
extern const char* gles2_search_paths[];
extern const char* gles1_search_paths[];
extern const char* egl_search_paths[];
extern const char* single_driver_search_paths[];

// Configuration files search paths
extern const char* applist_cfg_search_paths[];
extern const char* interceptor_cfg_search_paths[];
extern const char* fpsApplist_cfg_search_paths[];
#endif // !GLESLAYER

// Wrapper interface
namespace wrapper
{

    class CWrapper
    {
    public:
        static void log(const char *format, ...);
        static void* GetProcAddress(const char* procName);

        static bool sShowFPS;

#ifndef GLESLAYER
    private:
        static void LoadConfigFiles();
        static std::string getProcessName();

        static bool sDoIntercept;
        static std::string sInterceptorPath;
#endif
    };

#ifndef GLESLAYER
    const char* findFirst(const char** paths);
#endif
}

// Helper macros
#define SHORT_FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define DBG_LOG(format, ...) do { wrapper::CWrapper::log("%s,%d: " format, SHORT_FILE, __LINE__, ##__VA_ARGS__); } while (0)
#define PUBLIC __attribute__ ((visibility("default")))

#endif
