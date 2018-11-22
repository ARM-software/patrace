#include "common/library.hpp"
#include "common/os.hpp"

#if !defined(_WIN32)
#   ifndef _GNU_SOURCE
#       define _GNU_SOURCE // for dladdr
#   endif
#   include <dlfcn.h>
#else
#   include <windows.h>
#endif

void* GetFuncPtr(DLL_HANDLE dllHandle, const char* funcName)
{
    void* funcPtr = 0;
#ifdef WIN32
    funcPtr = GetProcAddress(dllHandle, funcName);
#else
    funcPtr = dlsym(dllHandle, funcName);
#endif
    return funcPtr;
}

DLL_HANDLE OpenDll(const char* dllName, const char *reqFunc)
{
    DLL_HANDLE handle = 0;
#ifdef WIN32
    handle = LoadLibraryA(dllName);
#else
    handle = dlopen(dllName, RTLD_NOW | RTLD_GLOBAL);
    if (handle)
    {
        void* funcPtr = dlsym(handle, reqFunc);
        if (!funcPtr)
        {
            DBG_LOG("Required function %s not found in %s!\n", reqFunc, dllName);
            return NULL;
        }
        Dl_info info;
        dladdr(funcPtr, &info);
        DBG_LOG("Loaded %s from %s\n", dllName, info.dli_fname);
    }
#endif
    return handle;
}

const char* GetDllError()
{
#ifdef WIN32
    return NULL;
#else
    return dlerror();
#endif
}

#ifndef __APPLE__

OpenCLLibrary::OpenCLLibrary()
: clGetPlatformIDs(NULL), clGetPlatformInfo(NULL),
  clGetDeviceIDs(NULL), clGetDeviceInfo(NULL),
  mInitialized(false)
{
}

bool OpenCLLibrary::Initialize()
{
    const char * dllname = "libOpenCL.so";
    DLL_HANDLE handle = OpenDll(dllname, "clGetDeviceInfo");
    if (handle == NULL)
    {
        //DBG_LOG("Failed to load library %s\n", dllname);
        return false;
    }


    clGetDeviceInfo = (clGetDeviceInfo_fn)GetFuncPtr(handle, "clGetDeviceInfo");
    clGetDeviceIDs = (clGetDeviceIDs_fn)GetFuncPtr(handle, "clGetDeviceIDs");
    clGetPlatformInfo = (clGetPlatformInfo_fn)GetFuncPtr(handle, "clGetPlatformInfo");
    clGetPlatformIDs = (clGetPlatformIDs_fn)GetFuncPtr(handle, "clGetPlatformIDs");

    if (!clGetDeviceInfo || !clGetDeviceIDs ||
        !clGetPlatformInfo || !clGetPlatformIDs)
    {
        //DBG_LOG("Failed to load function addresses of query objects\n");
        return false;
    }

    mInitialized = true;
    return true;
}

#endif
