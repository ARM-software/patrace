#if !defined(LIBRARY_HPP)
#define LIBRARY_HPP

#include <string>

#if !defined(_WIN32)
    typedef void* DLL_HANDLE;
#else
    #include <Windows.h>
    typedef HINSTANCE DLL_HANDLE;
#endif

DLL_HANDLE OpenDll(const char* dllName, const char* reqFunc);
const char* GetDllError();
void* GetFuncPtr(DLL_HANDLE dllHandle, const char* funcName);

#endif // !defined(LIBRARY_HPP)
