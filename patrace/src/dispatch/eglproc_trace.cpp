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


#include "eglproc_auto.hpp"
#include "os.hpp"
#include "common/library.hpp"
#include "os_string.hpp"

#if !defined(_WIN32)
#   ifndef _GNU_SOURCE
#       define _GNU_SOURCE // for dladdr
#   endif
#   include <dlfcn.h>

#   ifdef ANDROID
        struct libPathInfo {
            const char* eglLibPath;
            const char* gles1LibPath;
            const char* gles2LibPath;
        };

        static const libPathInfo libPaths[] = {
#ifdef __LP64__
            // 64-bit Mali drivers (/system)
            {
                "/system/lib64/egl/libGLES_mali.so",
                "/system/lib64/egl/libGLES_mali.so",
                "/system/lib64/egl/libGLES_mali.so"
            },
            // 64-bit Mali drivers (/vendor)
            {
                "/vendor/lib64/egl/libGLES_mali.so",
                "/vendor/lib64/egl/libGLES_mali.so",
                "/vendor/lib64/egl/libGLES_mali.so"
            },
            // 64-bit Mali drivers for Android 8 (/vendor)
            {
                "/vendor/lib64/egl/lib_mali.so",
                "/vendor/lib64/egl/lib_mali.so",
                "/vendor/lib64/egl/lib_mali.so"
            },
            // 64-bit Adreno drivers (as seen on Nexus 6P with Android N)
            {
                "/vendor/lib64/egl/libEGL_adreno.so",
                "/vendor/lib64/egl/libGLESv1_CM_adreno.so",
                "/vendor/lib64/egl/libGLESv2_adreno.so",
            },
#endif
            // first, attempt to load Android JB (Jellybean) libs for Mali-devices.
            {
                "/system/lib/egl/libEGL_mali.so",
                "/system/lib/egl/libGLESv1_CM_mali.so",
                "/system/lib/egl/libGLESv2_mali.so"
            },
            // second, attempt to load monolithic Android ICS (Ice Cream Sandwich) lib for Mali-devices.
            {
                "/system/lib/egl/libGLES_mali.so",
                "/system/lib/egl/libGLES_mali.so",
                "/system/lib/egl/libGLES_mali.so"
            },
            // If the previous attempt failed, try looking in a different location
            {
                "/vendor/lib/egl/libGLES_mali.so",
                "/vendor/lib/egl/libGLES_mali.so",
                "/vendor/lib/egl/libGLES_mali.so"
            },
            // 32-bit Mali drivers for Android 8 (/vendor)
            {
                "/vendor/lib/egl/lib_mali.so",
                "/vendor/lib/egl/lib_mali.so",
                "/vendor/lib/egl/lib_mali.so"
            },
            // 32-bit Adreno drivers (as seen on  Nexus 6P with Android N)
            {
                "/vendor/lib/egl/libEGL_adreno.so",
                "/vendor/lib/egl/libGLESv1_CM_adreno.so",
                "/vendor/lib/egl/libGLESv2_adreno.so",
            },
            // Last attempt, load generic gles libs, for tracing on non-Mali-devices.
            // tracing on non-Mali is not known to work.
            {
                "/system/lib/libEGL.so",
                "/system/lib/libGLESv1_CM.so",
                "/system/lib/libGLESv2.so"
            }
        };
#   else
#       define EGL_LIB_NAME "libEGL.so"
#       define GLES1_LIB_NAME "libGLESv1_CM.so"
#       define GLES2_LIB_NAME "libGLESv2.so"
#   endif

#else
#   include <windows.h>
    typedef HINSTANCE DLL_HANDLE;
#   define EGL_LIB_NAME "libEGL.dll"
#   define GLES1_LIB_NAME "libGLESv1_CM.dll"
#   define GLES2_LIB_NAME "libGLESv2.dll"
#endif

namespace {
    DLL_HANDLE gEGLHandle = 0;
    DLL_HANDLE gGLES2Handle = 0;
    DLL_HANDLE gGLES1Handle = 0;
    int gGLESVersion = 0;
};

void ResetGLFuncPtrs();

void SetGLESVersion(int ver)
{
    ResetGLFuncPtrs();
    gGLESVersion = ver;
}

int GetGLESVersion()
{
    return gGLESVersion;
}

enum DLLType {
    LibEGL,
    LibGLESv1,
    LibGLESv2
};

DLL_HANDLE OpenDllByType(enum DLLType t, const char* reqFunc)
{
    DLL_HANDLE ret = 0;
#ifdef ANDROID
    // foreach over the set of EGL/GLES1/GLES2 libraries
    // this is to support Android JB (Jellybean) and ICS (Ice Cream Sandwich) that have a different set of
    // GLES .so files (one per library (EGL, GLES1, GLES2), versus the monolithic libGLES.so.
    for(int i = 0; i < sizeof(libPaths) / sizeof(libPathInfo); i++) {
        const char * lib_filename;
        GetDllError();
        switch(t) {
            case LibEGL:
                lib_filename = libPaths[i].eglLibPath;
                ret = OpenDll(lib_filename, "eglInitialize");
                break;
            case LibGLESv1:
                lib_filename = libPaths[i].gles1LibPath;
                ret = OpenDll(lib_filename, "glGetString");
                break;
            case LibGLESv2:
                lib_filename = libPaths[i].gles2LibPath;
                ret = OpenDll(lib_filename, "glGetString");
                break;
            default:
                DBG_LOG("Invalid GL enum!\n");
                abort();
        }
        if (ret != 0) {
            break;
        } else {
            DBG_LOG("%s load failed: %s\n", lib_filename, GetDllError());
        }
    }
#else
    const char * lib_filename = 0;
    GetDllError(); // Clear error state
    switch(t) {
        case LibEGL:
            lib_filename = getenv("TRACE_LIBEGL");
            if(!lib_filename)
                lib_filename = EGL_LIB_NAME;
            ret = OpenDll(lib_filename, "eglInitialize");
            break;
        case LibGLESv1:
            lib_filename = getenv("TRACE_LIBGLES1");
            if(!lib_filename)
                lib_filename = GLES1_LIB_NAME;
            ret = OpenDll(lib_filename, "glGetString");
            break;
        case LibGLESv2:
            lib_filename = getenv("TRACE_LIBGLES2");
            if(!lib_filename)
                lib_filename = GLES2_LIB_NAME;
            ret = OpenDll(lib_filename, "glGetString");
            break;
        default:
            DBG_LOG("Invalid GL enum!\n");
            abort();
    }
    if (ret == 0)
    {
        DBG_LOG("%s load failed: %s\n", lib_filename, GetDllError());
    }
#endif
    return ret;
}

/*
 * Lookup a public EGL/GL/GLES symbol
 *
 * Look up with dlsym first, as this does not hurt, while eglGetProcAddress
 * can give an invalid return value if the function is unsupported on some
 * platforms.
 *
 * FIXME: This is a duplicate of same function in eglproc_retrace.cpp!
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

    if (retValue == NULL)
    {
        DBG_LOG("Cannot find the function pointer of %s\n", procName);
    }
    return retValue;
}
