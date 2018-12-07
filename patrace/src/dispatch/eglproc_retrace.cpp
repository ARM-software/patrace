/**************************************************************************
 *
 * Copyright 2011 Jose Fonseca
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/

#include "eglproc_retrace.hpp"

#include "eglproc_auto.hpp"
#include "os.hpp"
#include "common/library.hpp"
#include "os_string.hpp"
#include <string>
#include <unordered_set>

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif


#if defined(WIN32)
#   define EGL_LIB_NAME "libEGL.dll"
#   define GLES1_LIB_NAME "libGLESv1_CM.dll"
#   define GLES2_LIB_NAME "libGLESv2.dll"
#elif TARGET_IPHONE_SIMULATOR
#   define EGL_LIB_NAME ""
#   define GLES1_LIB_NAME "Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator5.1.sdk/System/Library/Frameworks/OpenGLES.framework/OpenGLES"
#   define GLES2_LIB_NAME "Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator5.1.sdk/System/Library/Frameworks/OpenGLES.framework/OpenGLES"
#elif TARGET_OS_IPHONE
#   define EGL_LIB_NAME ""
#   define GLES1_LIB_NAME "Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.1.sdk/System/Library/Frameworks/OpenGLES.framework/OpenGLES"
#   define GLES2_LIB_NAME "Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.1.sdk/System/Library/Frameworks/OpenGLES.framework/OpenGLES"
#else
// Android and Linux-es
#   define EGL_LIB_NAME "libEGL.so"
#   define GLES1_LIB_NAME "libGLESv1_CM.so"
#   define GLES2_LIB_NAME "libGLESv2.so"
#endif

#ifdef ANDROID
struct libPathInfo {
    std::vector<std::string> eglLibPath;
    std::vector<std::string> gles1LibPath;
    std::vector<std::string> gles2LibPath;
};

#ifdef PLATFORM_64BIT
static const libPathInfo libPaths = {
    .eglLibPath = { "/sdcard/libEGL_nomali_arm.so", "/system/lib64/libEGL.so", },
    .gles1LibPath = { "/sdcard/libGLESv1_CM_nomali_arm.so", "/system/lib64/libGLESv1_CM.so", },
    .gles2LibPath = { "/sdcard/libGLESv2_nomali_arm.so", "/system/lib64/libGLESv2.so" }
};
#else
static const libPathInfo libPaths = {
    .eglLibPath = { "/sdcard/libEGL_nomali_arm64.so",  "/system/lib/libEGL.so", },
    .gles1LibPath = { "/sdcard/libGLESv1_CM_nomali_arm64.so", "/system/lib/libGLESv1_CM.so", },
    .gles2LibPath = { "/sdcard/libGLESv2_nomali_arm64.so", "/system/lib/libGLESv2.so" }
};
#endif

#endif

static struct CommandLineSettings{
    std::string libEGL_path;
    std::string libGLESv1_path;
    std::string libGLESv2_path;
} gCommandLineSettings;

void SetCommandLineEGLPath(const std::string& libEGL_path) {
    gCommandLineSettings.libEGL_path = libEGL_path;
}

void SetCommandLineGLES1Path(const std::string& libGLESv1_path) {
    gCommandLineSettings.libGLESv1_path = libGLESv1_path;
}

void SetCommandLineGLES2Path(const std::string& libGLESv2_path) {
    gCommandLineSettings.libGLESv2_path = libGLESv2_path;
}

namespace {
    DLL_HANDLE gEGLHandle = 0;
    DLL_HANDLE gGLES2Handle = 0;
    DLL_HANDLE gGLES1Handle = 0;
    int gGLESVersion = 0;
};

void ResetGLFuncPtrs();

extern "C"
void SetGLESVersion(int ver)
{
    if (gGLESVersion == ver)
        return;

    DBG_LOG("Set GLES version to be: %d\n", ver);
    ResetGLFuncPtrs();
    gGLESVersion = ver;
}

enum DLLType {
    LibEGL,
    LibGLESv1,
    LibGLESv2
};

DLL_HANDLE OpenDllByType(enum DLLType t, const char *reqFunc)
{
    DLL_HANDLE ret = 0;
    GetDllError(); // Clear error state
    std::string lib_filename = "(none)";
#ifdef ANDROID
    // why env vars dont work well with java:
    // http://stackoverflow.com/questions/7597058/android-setget-environmental-variables-in-java
    const std::vector<std::string>* list = nullptr;
    switch(t) {
        case LibEGL:
            list = &libPaths.eglLibPath;
            break;
        case LibGLESv1:
            list = &libPaths.gles1LibPath;
            break;
        case LibGLESv2:
            list = &libPaths.gles2LibPath;
            break;
    }
    for (const std::string& path : *list)
    {
        lib_filename = path;
        DLL_HANDLE ret = OpenDll(lib_filename.c_str(), reqFunc);
        if (ret != 0)
        {
            return ret;
        }
    }
#elif TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    switch (t) {
        case LibEGL:
            return 0; //No egl lib on iOS
        case LibGLESv1:
            lib_filename = GLES1_LIB_NAME;
            break;
        case LibGLESv2:
            lib_filename = GLES2_LIB_NAME;
            break;
    }
    ret = OpenDll(lib_filename.c_str(), reqFunc);
#else // (desktop/embedded Linux)
    switch(t) {
        case LibEGL:
            if( !gCommandLineSettings.libGLESv2_path.empty() ) {
                lib_filename = gCommandLineSettings.libGLESv2_path.c_str();
            } else {
                lib_filename = EGL_LIB_NAME;
            }
            break;
        case LibGLESv1:
            if( !gCommandLineSettings.libGLESv2_path.empty() ) {
                lib_filename = gCommandLineSettings.libGLESv2_path.c_str();
            } else {
                lib_filename = GLES1_LIB_NAME;
            }
            break;
        case LibGLESv2:
            if( !gCommandLineSettings.libGLESv2_path.empty() ) {
                lib_filename = gCommandLineSettings.libGLESv2_path.c_str();
            }
            else {
                lib_filename = GLES2_LIB_NAME;
            }
            break;
    }
    ret = OpenDll(lib_filename.c_str(), reqFunc);
#endif
    if (ret == 0)
    {
        DBG_LOG("%s load failed: %s\n", lib_filename.c_str(), GetDllError());
    }
    return ret;
}

/// Register each function for which we have complained about lacking
/// function pointer, to avoid spamming endlessly about it.
static std::unordered_set<std::string> complained;

/*
 * Lookup a public EGL/GL/GLES symbol
 *
 * Look up with dlsym first, as this does not hurt, while eglGetProcAddress
 * can give an invalid return value if the function is unsupported on some
 * platforms.
 */
void* _getProcAddress(const char* procName)
{
    void* retValue = NULL;

    if (gEGLHandle == NULL || gGLES2Handle == NULL)
    {
        // for ARM GLES 3.0 emulator, a symbol needed by libEGL
        // is defined in libGLESv2, therefore, load libGLESv2 first
        gGLES2Handle = OpenDllByType(LibGLESv2, "glGetString");
        gEGLHandle = OpenDllByType(LibEGL, "eglInitialize");
    }

    if (gGLESVersion == 1 && gGLES1Handle == NULL)
    {
        gGLES1Handle = OpenDllByType(LibGLESv1, "glGetString");
    }

    if (procName && procName[0]=='e')
    {
        retValue = GetFuncPtr(gEGLHandle, procName);
    }
    else if (procName && procName[0]=='g')
    {
        if (gGLESVersion == 1)
        {
            retValue = GetFuncPtr(gGLES1Handle, procName);
        }
        else
        {
            retValue = GetFuncPtr(gGLES2Handle, procName);
        }
    }

    if (retValue == NULL) // fallback to using eglGetProcAddress
    {
        retValue = (void *)_eglGetProcAddress(procName);
    }

    if (retValue == NULL && complained.count(procName) == 0)
    {
        DBG_LOG("Cannot find the function pointer of %s\n", procName);
        complained.insert(procName);
    }
    return retValue;
}
