#include "DynamicLibrary.hpp"
#include "common/os.hpp"

#include <iostream>
#include <dlfcn.h>

DynamicLibrary::DynamicLibrary(const char *fileName)
{
    libHandle = dlopen(fileName, RTLD_LAZY);
    if (!libHandle)
    {
        DBG_LOG("Open file %s failed: %s\n", fileName, dlerror());
    }
    else
    {
        DBG_LOG("Open file %s successfully, libHandle = %p\n", fileName, libHandle);
    }
}

DynamicLibrary::~DynamicLibrary()
{
    if (libHandle) dlclose(libHandle);
}

void *DynamicLibrary::getFunctionPtr(const char *name) const
{
    auto ret = (void *)dlsym(libHandle, name);
    if (ret == nullptr)
    {
        DBG_LOG("Open function %s failed: %s\n", name, dlerror());
    }
    else
    {
        DBG_LOG("Open function %s successfully, ret = %p\n", name, ret);
    }
    return ret;
}
