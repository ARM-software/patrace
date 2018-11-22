#include "DynamicLibrary.hpp"
#include "common/os.hpp"

#include <iostream>
#include <dlfcn.h>

DynamicLibrary::DynamicLibrary(const char *fileName)
{
    libHandle = dlopen(fileName, RTLD_LAZY);
    if (!libHandle)
    {
        DBG_LOG("====================open file %s failed\n", fileName);
        throw OpenLibFailedException();
    }
    else
        DBG_LOG("====================open file %s successfully, libHandle = %p\n", fileName, libHandle);
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
        DBG_LOG("====================open function %s failed\n", name);
        std::cerr << "Failed to get function " << name << std::endl;
    }
    else
        DBG_LOG("====================open function %s successfully, ret = %p\n", name, ret);
    return ret;
}
