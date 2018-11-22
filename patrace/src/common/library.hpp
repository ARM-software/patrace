#if !defined(LIBRARY_HPP)
#define LIBRARY_HPP

#include <string>

#ifndef __APPLE__
#define CL_TARGET_OPENCL_VERSION 200
#include "CL/opencl.h"
#endif

#if !defined(_WIN32)
    typedef void* DLL_HANDLE;
#else
    #include <Windows.h>
    typedef HINSTANCE DLL_HANDLE;
#endif

DLL_HANDLE OpenDll(const char* dllName, const char* reqFunc);
const char* GetDllError();
void* GetFuncPtr(DLL_HANDLE dllHandle, const char* funcName);

#ifndef __APPLE__

class OpenCLLibrary
{
public:
    OpenCLLibrary();

    bool Initialize();
    bool IsInitialized() const { return mInitialized; }

    typedef CL_API_ENTRY cl_int (CL_API_CALL *clGetPlatformIDs_fn)(cl_uint num_entries, cl_platform_id *platforms, cl_uint *num_platforms);
    typedef CL_API_ENTRY cl_int (CL_API_CALL *clGetPlatformInfo_fn)(cl_platform_id platform, cl_platform_info param_name, size_t param_value_size, void *param_value, size_t *param_value_size_ret);
    typedef CL_API_ENTRY cl_int (CL_API_CALL *clGetDeviceIDs_fn)(cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id *devices, cl_uint *num_devices);
    typedef CL_API_ENTRY cl_int (CL_API_CALL *clGetDeviceInfo_fn)(cl_device_id device, cl_device_info param_name, size_t param_value_size, void *param_value, size_t *param_value_size_ret);

    clGetPlatformIDs_fn clGetPlatformIDs;
    clGetPlatformInfo_fn clGetPlatformInfo;
    clGetDeviceIDs_fn clGetDeviceIDs;
    clGetDeviceInfo_fn clGetDeviceInfo;

private:
    bool mInitialized;
};

#endif

#endif // !defined(LIBRARY_HPP)
