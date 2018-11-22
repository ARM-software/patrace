#include "../common.h"

#include "EGL/egl.h"

using namespace wrapper;

extern "C" {

PUBLIC __eglMustCastToProperFunctionPointerType eglGetProcAddress(const char *procname);

typedef __eglMustCastToProperFunctionPointerType (*FUNCPTR_eglGetProcAddress)(const char *procname);
/// We never cache this function, to avoid Android putting it into its cache and making us lose
/// control of it when we want to start tracing.
__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char *procname)
{
    FUNCPTR_eglGetProcAddress sp_eglGetProcAddress = (FUNCPTR_eglGetProcAddress) wrapper::CWrapper::GetProcAddress("eglGetProcAddress");
    if (sp_eglGetProcAddress == 0)
    {
        return 0;
    }
    return sp_eglGetProcAddress(procname);
}

}
