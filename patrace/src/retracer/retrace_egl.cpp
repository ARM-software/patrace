#include <retracer/retracer.hpp>
#include <retracer/glws_egl.hpp>
#include <retracer/retrace_api.hpp>
#include <retracer/forceoffscreen/offscrmgr.h>
#include <common/gl_extension_supported.hpp>
#include <common/trace_limits.hpp>
#include <helper/eglstring.hpp>
#include "../dispatch/eglproc_auto.hpp"
#include "dma_buffer/dma_buffer.hpp"

#define EGL_EGLEXT_PROTOTYPES
#include "EGL/eglext.h"

#define EGL_CONTEXT_CLIENT_VERSION	0x3098

using namespace common;
using namespace retracer;

static void callback(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length, const char* message, const void* userParam)
{
    if (type == GL_DEBUG_TYPE_PERFORMANCE) return; // too much
    DBG_LOG("%s::%s::%s (call=%u frame=%u) %s: %s\n", cbsource(source), cbtype(type), cbseverity(severity), gRetracer.GetCurCallId(), gRetracer.GetCurFrameId(), gRetracer.GetCurCallName(), message);
}

void getSurfaceDimensions(int* width, int* height)
{
    const RetraceOptions& opt = gRetracer.mOptions;
    if(opt.mForceOffscreen)
    {
        // In offscreen only create a window surface large enough to hold the mosaic.
        // This is because allocating a larger window surface than available screen will fail on some platforms (Mali linux fbdev).
        *width = opt.mOnscrSampleW * opt.mOnscrSampleNumX;
        *height = opt.mOnscrSampleH * opt.mOnscrSampleNumY;
    }
    else if (opt.mDoOverrideResolution)
    {
        *width = opt.mOverrideResW;
        *height = opt.mOverrideResH;
    }
    else
    {
        *width = opt.mWindowWidth;
        *height = opt.mWindowHeight;
    }
}

void getLocalGLESVersion()
{
    RetraceOptions& opt = gRetracer.mOptions;

    const char* version = (const char*)glGetString(GL_VERSION);
    if (strstr(version, "ES 3.2") != NULL) {
        opt.mLocalApiVersion = PROFILE_ES32;
    }
    else if (strstr(version, "ES 3.1") != NULL) {
        opt.mLocalApiVersion = PROFILE_ES31;
    }
    else if (strstr(version, "ES 3.") != NULL) {
        opt.mLocalApiVersion = PROFILE_ES3;
    }
    else if (strstr(version, "ES 2.") != NULL) {
        opt.mLocalApiVersion = PROFILE_ES2;
    }
    else if (strstr(version, "ES 1.") != NULL){
        opt.mLocalApiVersion = PROFILE_ES1;
    }
    else {
        opt.mLocalApiVersion = PROFILE_ESX;
    }
    DBG_LOG("Version: %s\n", version);
}

#ifdef ANDROID
static inline uint32_t get_drm_format(EGLDisplay display, EGLConfig config)
{
    uint32_t drm_format = 0;
    EGLint order;
    EGLint num_of_planes;
    EGLint subsample;
    EGLint plane_bpp;

    eglGetConfigAttrib(display, config, EGL_YUV_ORDER_EXT, &order);
    eglGetConfigAttrib(display, config, EGL_YUV_NUMBER_OF_PLANES_EXT, &num_of_planes);
    eglGetConfigAttrib(display, config, EGL_YUV_SUBSAMPLE_EXT, &subsample);
    eglGetConfigAttrib(display, config, EGL_YUV_PLANE_BPP_EXT, &plane_bpp);

    if (order == EGL_YUV_ORDER_YUV_EXT)
    {
        if (num_of_planes == 2)
        {
            if (subsample == EGL_YUV_SUBSAMPLE_4_2_0_EXT && plane_bpp == 8)         drm_format = DRM_FORMAT_NV12;
            else if (subsample == EGL_YUV_SUBSAMPLE_4_2_2_EXT && plane_bpp == 8)    drm_format = DRM_FORMAT_NV16;
            else if (subsample == EGL_YUV_SUBSAMPLE_4_2_0_EXT && plane_bpp == 10)   drm_format = DRM_FORMAT_P010;
            else if (subsample == EGL_YUV_SUBSAMPLE_4_2_2_EXT && plane_bpp == 10)   drm_format = DRM_FORMAT_P210;
        }
        else if (num_of_planes == 3)
        {
            if (subsample == EGL_YUV_SUBSAMPLE_4_4_4_EXT && plane_bpp == 8)         drm_format = DRM_FORMAT_YUV444;
            else if (subsample == EGL_YUV_SUBSAMPLE_4_4_4_EXT && plane_bpp == 10)   drm_format = DRM_FORMAT_Q410;
            else if (subsample == EGL_YUV_SUBSAMPLE_4_2_0_EXT && plane_bpp == 8)    drm_format = DRM_FORMAT_YUV420;
        }
    }
    else if (order == EGL_YUV_ORDER_YVU_EXT && subsample == EGL_YUV_SUBSAMPLE_4_2_0_EXT && plane_bpp == 8)
    {
        if (num_of_planes == 2)       drm_format = DRM_FORMAT_NV21;
        else if (num_of_planes == 3)  drm_format = DRM_FORMAT_YVU420;
    }

    return drm_format;
}

static bool hardcode_egl_query_compression_rates(EGLDisplay display, EGLConfig config, const EGLAttrib *attrib_list, EGLint *rates, EGLint rate_size, EGLint *num_rates)
{
    std::string ext = _eglQueryString(display, EGL_EXTENSIONS);

    if (ext.find("EGL_EXT_surface_compression") == std::string::npos)
    {
        *num_rates = 0;
        DBG_LOG("WARNING: EGL_EXT_surface_compression is not exposed on this platform.\n");
        return true;
    }

    // Mali GPU
    EGLint buffer_type;
    EGLint red_size = 0;
    EGLint green_size = 0;
    EGLint blue_size = 0;
    EGLint alpha_size = 0;

    eglGetConfigAttrib(display, config, EGL_COLOR_BUFFER_TYPE, &buffer_type);
    eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red_size);
    eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green_size);
    eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue_size);
    eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &alpha_size);

    // 1. check config valid to afrc
    bool valid_config = false;
    if (EGL_YUV_BUFFER_EXT == buffer_type)    // yuv buffer
    {
        uint32_t yuv_drm = get_drm_format(display, config);
        switch (yuv_drm)
        {
            case DRM_FORMAT_NV12:
            case DRM_FORMAT_NV16:
            case DRM_FORMAT_YUV444:
            case DRM_FORMAT_YUV420:
            case DRM_FORMAT_Q410:
            case DRM_FORMAT_P010:
            case DRM_FORMAT_P210:
            case DRM_FORMAT_NV21:
            case DRM_FORMAT_NV61:
            case DRM_FORMAT_YVU444:
            case DRM_FORMAT_YVU420:
            case DRM_FORMAT_Q401:
                valid_config = true;
                break;
            default:
                break;
        }
    }
    else
    {
        if ( ((8 == red_size) && (8 == green_size) && (8 == blue_size)) ||
             ((5 == red_size) && (6 == green_size) && (5 == blue_size)) )
        {
            // TODO fix me!!!
            // According to ddk, if arm_config_type equals EGLP_ARM_CONFIG_YUV_SPECIAL_BIT, config is invalid for afrc.
            // But patrace could not get EGLP_ARM_CONFIG_TYPE through eglGetConfigAttrib.
            valid_config = true;
        }
    }

    //2. check attrib valid to afrc to exclude sRGB.
    bool is_srgb = false;
    if (attrib_list != NULL)
    {
        const EGLint *cur_attrib = (const EGLint *)attrib_list;
        while (EGL_NONE != *cur_attrib)
        {
            if (EGL_GL_COLORSPACE_KHR == *cur_attrib)
            {
                is_srgb =  *(cur_attrib+1) == EGL_GL_COLORSPACE_SRGB_KHR;
                break;
            }
            cur_attrib += 2;
        }
    }

    if (!valid_config || is_srgb)
    {
        *num_rates = 0;
        return true;
    }

    //3. get compression rates
    const EGLint low_afrc_rate[] = { EGL_SURFACE_COMPRESSION_FIXED_RATE_2BPC_EXT,
                                     EGL_SURFACE_COMPRESSION_FIXED_RATE_4BPC_EXT,
                                     EGL_SURFACE_COMPRESSION_FIXED_RATE_5BPC_EXT };
    const EGLint high_afrc_rate[] = { EGL_SURFACE_COMPRESSION_FIXED_RATE_2BPC_EXT,
                                      EGL_SURFACE_COMPRESSION_FIXED_RATE_3BPC_EXT,
                                      EGL_SURFACE_COMPRESSION_FIXED_RATE_4BPC_EXT };
    *num_rates = 3;
    if (rate_size == 0 || rates == NULL)
    {
        return true;
    }

    const EGLint *tmp_rates;

    // 3-component(RGB) AFRC has low compression efficiency.
    if ((0 != red_size) && (0 != green_size) && (0 != blue_size) && (0 == alpha_size))
    {
        tmp_rates =  low_afrc_rate;
    }
    else
    {
        tmp_rates = high_afrc_rate;
    }
    if (rate_size < *num_rates)
    {
        *num_rates = rate_size;
    }

    for (int i = 0; i < *num_rates; i++)
    {
        rates[i] = tmp_rates[i];
    }

    return true;
}
#endif

static bool convertToBPC(int flag, compressionControlInfo &compressInfo, EGLint const* attrib_list)
{
#ifdef ANDROID
    // MPGPPAP-5290: patrace fails to get the new extension function pointer through eglGetProcAddress because Android libEGL.so only support the current display to call extension
    // through eglGetProcAddress. Otherwise it fails due to the null current context.
    // To work around, hard code the BPC convertion on Android.
    hardcode_egl_query_compression_rates(gRetracer.mState.mEglDisplay, gRetracer.mState.mEglConfig, reinterpret_cast<EGLAttrib const*>(attrib_list), NULL, 0, &compressInfo.rate_size);
#else
    GLWS::instance().querySupportedCompressionRates(reinterpret_cast<EGLAttrib const*>(attrib_list), NULL, 0, &compressInfo.rate_size);
#endif
    if (compressInfo.rate_size == 0) {
        DBG_LOG("!!!!!! WARNING: rate_size is 0. No supported fixed rate queried on eglSurface.\n");
        return false;
    }

    std::vector<int> rates(compressInfo.rate_size);
#ifdef ANDROID
    hardcode_egl_query_compression_rates(gRetracer.mState.mEglDisplay, gRetracer.mState.mEglConfig, reinterpret_cast<EGLAttrib const*>(attrib_list), rates.data(), compressInfo.rate_size, &compressInfo.rate_size);
#else
    GLWS::instance().querySupportedCompressionRates(reinterpret_cast<EGLAttrib const*>(attrib_list), rates.data(), compressInfo.rate_size, &compressInfo.rate_size);
#endif
    compressInfo.rates = rates.data();
    getCompressionInfo(flag, true, compressInfo);

    return true;
}

PUBLIC void retrace_eglCreateWindowSurface(char* src)
{
    // ------- ret & params definition --------
    int ret;
    int dpy;
    int config;
    int win;
    common::Array<int> attrib_list;

    // --------- read ret & params ----------
    src = ReadFixed(src, dpy);
    src = ReadFixed(src, config);
    src = ReadFixed(src, win);
    src = Read1DArray(src, attrib_list);

    src = ReadFixed(src, ret);

    const RetraceOptions& opt = gRetracer.mOptions;
    GLESThread& thread = gRetracer.mState.mThreadArr[gRetracer.getCurTid()];
    //  ------------- retrace ---------------
    //DBG_LOG("[%d] retrace_eglCreateWindowSurface %d, %d\n", gRetracer.getCurTid(), config, ret);
    thread.mCurAppVP.x = 0;
    thread.mCurAppVP.y = 0;
    thread.mCurAppVP.w = opt.mWindowWidth;
    thread.mCurAppVP.h = opt.mWindowHeight;
    thread.mCurAppSR = thread.mCurAppVP;

    if (opt.mDoOverrideResolution)
    {
        thread.mCurDrvVP.x = 0;
        thread.mCurDrvVP.y = 0;
        thread.mCurDrvVP.w = opt.mOverrideResW;
        thread.mCurDrvVP.h = opt.mOverrideResH;
        thread.mCurDrvSR = thread.mCurDrvVP;
        //DBG_LOG("Setting scissor to: %d %d\n", gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurDrvSR.w, gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurDrvSR.h);
    }

    if (ret == 0) // ret == EGL_NO_SURFACE
    {
        return;
    }

    StateMgr& s = gRetracer.mState;

    if (gRetracer.mOptions.mForceSingleWindow && !gRetracer.mOptions.mMultiThread)
    {
        win = 0;

        if (s.mSingleSurface)
        {
            s.InsertDrawableMap(ret, s.mSingleSurface);
            DBG_LOG("INFO: Ignoring creation of surface %d: 'forceSingleWindow' is set to 'true' in the header of the trace file\n", ret);
            return;
        }
        else DBG_LOG("'forceSingleWindow' is set to true but master window not yet created, not setting surface %d\n", ret); // very bad
    }

    int width = 0;
    int height = 0;
    getSurfaceDimensions(&width, &height);

    retracer::Drawable* d;
    if (gRetracer.mOptions.mPbufferRendering || (gRetracer.mOptions.mSingleSurface != -1 && gRetracer.mOptions.mSingleSurface != gRetracer.mSurfaceCount))
    {
        DBG_LOG("[%d] Creating Pbuffer drawable for surface %d: w=%d, h=%d...\n", gRetracer.getCurTid(), ret, width, height);
        EGLint const attribs[] = { EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE, EGL_NONE };
        d = GLWS::instance().CreatePbufferDrawable(attribs);
    }
    else
    {
        DBG_LOG("[%d] Creating drawable for surface %d: w=%d, h=%d...\n", gRetracer.getCurTid(), ret, width, height);
        if (gRetracer.mOptions.eglAfrcRate != -1) {
            compressionControlInfo compressInfo = {0};
            if (convertToBPC(gRetracer.mOptions.eglAfrcRate, compressInfo, attrib_list) == true) {
                std::vector<int> new_attrib_list = AddtoAttribList(attrib_list, compressInfo.extension, compressInfo.rate, EGL_NONE);
                //if (gRetracer.mOptions.mDebug > 0)
                    DBG_LOG("Enable fixed rate attrib(0x%04x, 0x%04x) on eglSurface in eglCreateWindowSurface().\n", compressInfo.extension, compressInfo.rate);
                d = GLWS::instance().CreateDrawable(width, height, win, new_attrib_list.data());
            }
            else {
                d = GLWS::instance().CreateDrawable(width, height, win, attrib_list);
            }
        }
        else {
            d = GLWS::instance().CreateDrawable(width, height, win, attrib_list);
        }
    }
    gRetracer.mSurfaceCount++;
    s.InsertDrawableMap(ret, d);
    s.InsertDrawableToWinMap(ret, win);

    d->winWidth  = opt.mWindowWidth;
    d->winHeight = opt.mWindowHeight;
    if (opt.mDoOverrideResolution)
    {
        d->mOverrideResRatioW = opt.mOverrideResW / (float) opt.mWindowWidth;
        d->mOverrideResRatioH = opt.mOverrideResH / (float) opt.mWindowHeight;
    }

    if (gRetracer.mOptions.mForceSingleWindow && !gRetracer.mOptions.mMultiThread)
    {
        s.mSingleSurface = d;
    }
}

PUBLIC void retrace_eglCreateWindowSurface2(char* src)
{
    // ------- ret & params definition --------
    int ret;
    int dpy;
    int config;
    int win;
    int x;
    int y;
    int surfWidth;
    int surfHeight;
    int winWidth;
    int winHeight;
    common::Array<int> attrib_list;

    // --------- read ret & params ----------
    src = ReadFixed(src, dpy);
    src = ReadFixed(src, config);
    src = ReadFixed(src, win);
    src = Read1DArray(src, attrib_list);
    src = ReadFixed(src, x);
    src = ReadFixed(src, y);
    src = ReadFixed(src, winWidth);
    src = ReadFixed(src, winHeight);

    surfWidth = winWidth;
    surfHeight = winHeight;

    src = ReadFixed(src, ret);

    const RetraceOptions& opt = gRetracer.mOptions;
    GLESThread& thread = gRetracer.mState.mThreadArr[gRetracer.getCurTid()];
    //  ------------- retrace ---------------
    //DBG_LOG("[%d] retrace_eglCreateWindowSurface %d, %d\n", gRetracer.getCurTid(), config, ret);
    thread.mCurAppVP.x = x;
    thread.mCurAppVP.y = y;
    thread.mCurAppVP.w = winWidth;
    thread.mCurAppVP.h = winHeight;
    thread.mCurAppSR = thread.mCurAppVP;

    if (opt.mDoOverrideResolution)
    {
        thread.mCurDrvVP.x = 0;
        thread.mCurDrvVP.y = 0;
        thread.mCurDrvVP.w = opt.mOverrideResW;
        thread.mCurDrvVP.h = opt.mOverrideResH;
        thread.mCurDrvSR = thread.mCurDrvVP;
        //DBG_LOG("Setting scissor to: %d %d\n", gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurDrvSR.w, gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurDrvSR.h);
    }

    if (ret == 0) // ret == EGL_NO_SURFACE
    {
        return;
    }

    StateMgr& s = gRetracer.mState;

    if (gRetracer.mOptions.mForceSingleWindow && !gRetracer.mOptions.mMultiThread)
    {
        win = 0;

        if (s.mSingleSurface)
        {
            s.InsertDrawableMap(ret, s.mSingleSurface);
            DBG_LOG("INFO: Ignoring creation of surface %d: 'forceSingleWindow' is set to 'true' in the header of the trace file\n", ret);
            return;
        }
        else DBG_LOG("'forceSingleWindow' is set to true but master window not yet created, not setting surface %d\n", ret); // very bad
    }

    if (opt.mForceOffscreen || (opt.mForceSingleWindow && !gRetracer.mOptions.mMultiThread) || opt.mDoOverrideResolution)
    {
        getSurfaceDimensions(&surfWidth, &surfHeight);
    }

    retracer::Drawable* d;
    if (gRetracer.mOptions.mPbufferRendering || (gRetracer.mOptions.mSingleSurface != -1 && gRetracer.mOptions.mSingleSurface != gRetracer.mSurfaceCount))
    {
        DBG_LOG("[%d] Creating Pbuffer drawable for surface %d: x=%d, y=%d, w=%d, h=%d...\n", gRetracer.getCurTid(), ret, x, y, surfWidth, surfHeight);
        EGLint const attribs[] = { EGL_WIDTH, surfWidth, EGL_HEIGHT, surfHeight, EGL_NONE, EGL_NONE };
        d = GLWS::instance().CreatePbufferDrawable(attribs);
    }
    else
    {
        DBG_LOG("[%d] Creating drawable for surface %d: x=%d, y=%d, w=%d, h=%d...\n", gRetracer.getCurTid(), ret, x, y, surfWidth, surfHeight);
        if (gRetracer.mOptions.eglAfrcRate != -1) {
            compressionControlInfo compressInfo = {0};
            if (convertToBPC(gRetracer.mOptions.eglAfrcRate, compressInfo, attrib_list) == true) {
                std::vector<int> new_attrib_list = AddtoAttribList(attrib_list, compressInfo.extension, compressInfo.rate, EGL_NONE);
                //if (gRetracer.mOptions.mDebug > 0)
                    DBG_LOG("Enable fixed rate attrib(0x%04x, 0x%04x) on eglSurface in eglCreateWindowSurface2().\n", compressInfo.extension, compressInfo.rate);
                d = GLWS::instance().CreateDrawable(surfWidth, surfHeight, win, new_attrib_list.data());
            }
            else {
                d = GLWS::instance().CreateDrawable(surfWidth, surfHeight, win, attrib_list);
            }
        }
        else {
            d = GLWS::instance().CreateDrawable(surfWidth, surfHeight, win, attrib_list);
        }
    }
    gRetracer.mSurfaceCount++;
    s.InsertDrawableMap(ret, d);

    d->winWidth  = winWidth;
    d->winHeight = winHeight;
    if (opt.mDoOverrideResolution)
    {
        d->mOverrideResRatioW = opt.mOverrideResW / (float) winWidth;
        d->mOverrideResRatioH = opt.mOverrideResH / (float) winHeight;
    }

    if (gRetracer.mOptions.mForceSingleWindow && !gRetracer.mOptions.mMultiThread)
    {
        s.mSingleSurface = d;
    }
}

PUBLIC void retrace_eglCreatePbufferSurface(char* src)
{
    // ------- ret & params definition --------
    int ret;
    int dpy;
    int config;
    common::Array<int> attrib_list;

    // --------- read ret & params ----------
    src = ReadFixed(src, dpy);
    src = ReadFixed(src, config);
    src = Read1DArray(src, attrib_list);
    src = ReadFixed(src, ret);

    //  ------------- retrace ---------------
    DBG_LOG("[%d] retrace_eglCreatePbufferSurface %d, %d\n", gRetracer.getCurTid(), config, ret);

    if (ret == 0) // ret == EGL_NO_SURFACE
    {
        return;
    }

    retracer::Drawable* pbufferDrawable = GLWS::instance().CreatePbufferDrawable(attrib_list);
    gRetracer.mState.InsertDrawableMap(ret, pbufferDrawable);
}

PUBLIC void retrace_eglDestroySurface(char* src)
{
    gRetracer.OnFrameComplete();

    // ------- ret & params definition --------
    int ret;
    int dpy;
    int surface;

    // --------- read ret & params ----------
    src = ReadFixed(src, dpy);
    src = ReadFixed(src, surface);
    src = ReadFixed(src, ret);

    //  ------------- retrace ---------------
    //DBG_LOG("CallNo[%d] retrace_eglDestroySurface()\n", gRetracer.GetCurCallId());

    StateMgr& s = gRetracer.mState;

    retracer::Drawable* toBeDel = 0;

    if (gRetracer.mOptions.mForceSingleWindow && !gRetracer.mOptions.mMultiThread)
    {
        toBeDel = s.mSingleSurface;
    }
    else
    {
        toBeDel = s.GetDrawable(surface);
    }

    s.RemoveDrawableMap(surface);

    if (toBeDel != NULL && toBeDel != s.mThreadArr[gRetracer.getCurTid()].getDrawable()
        && !s.IsInDrawableMap(toBeDel))
    {
        toBeDel->release();
        toBeDel = 0;
        s.mSingleSurface = 0;
    }
}

PUBLIC void retrace_eglBindAPI(char* src)
{
    // ------- ret & params definition --------
    int ret;
    int api;

    // --------- read ret & params ----------
    src = ReadFixed(src, api);
    src = ReadFixed(src, ret);

    //  ------------- retrace ---------------
    //eglBindAPI(api);
}

PUBLIC void retrace_eglCreateContext(char* src)
{
    // ------- ret & params definition --------
    int ret;
    int dpy;
    int config;
    int share_context;
    common::Array<int> attrib_list;

    // --------- read ret & params ----------
    src = ReadFixed(src, dpy);
    src = ReadFixed(src, config);
    src = ReadFixed(src, share_context);
    src = Read1DArray(src, attrib_list);
    src = ReadFixed(src, ret);

    // If the original content failed to create a context here, avoid creating it in replay as well.
    if (ret == 0) return; // ret == EGL_NO_CONTEXT

    //  ------------- retrace ---------------
    Context* pshare_context = gRetracer.mState.GetContext(share_context);
    Profile profile = PROFILE_ES1;
    int major = 1, minor = 0;
    for (unsigned int i = 0; i < attrib_list.cnt; i += 2) {
        if (attrib_list.v[i] == EGL_CONTEXT_CLIENT_VERSION) {
            switch (attrib_list.v[i+1])
            {
            case 2:
                major = 2;
                break;
            case 3:
                major = 3;
                break;
            default:
                DBG_LOG("WARN: unknown GLES version %d, in default, set it to GLES 1\n", attrib_list.v[i+1]);
            }
        }
        if (attrib_list.v[i] == EGL_CONTEXT_MINOR_VERSION) {
            minor = attrib_list.v[i+1];
        }
    }

    // Applications that are actually GLES3 may decide to say they want a GLES2 context.
    // This is ok for most Android devices, but causes problems for our desktop GLES2/3 emulator that
    // is strict about what functions will execute depending on the requested EGL_CONTEXT_CLIENT_VERSION.
    // So, instead of trusting the app, lets explicitly choose the ctx version based on the trace header
    // for the GLES2 context.
    if (major == 2)
    {
        profile = gRetracer.mOptions.mApiVersion;
    }
    else if (major == 3)
    {
        switch(minor)
        {
        case 0:
            profile = PROFILE_ES3;
            break;
        case 1:
            profile = PROFILE_ES31;
            break;
        case 2:
            profile = PROFILE_ES32;
            break;
        }
    }

    SetGLESVersion(profile);

    Context* context = GLWS::instance().CreateContext(pshare_context, profile);
    if (!context)
    {
        gRetracer.reportAndAbort("Failed to create GLES (%d) context", profile);
    }

    gRetracer.mState.InsertContextMap(ret, context);
}

PUBLIC void retrace_eglDestroyContext(char* src)
{
    // ------- ret & params definition --------
    int ret;
    int dpy;
    int ctx;

    // --------- read ret & params ----------
    src = ReadFixed(src, dpy);
    src = ReadFixed(src, ctx);
    src = ReadFixed(src, ret);

    if (gRetracer.mOptions.mForceOffscreen && gRetracer.mMosaicNeedToBeFlushed) {
        DBG_LOG("Render the mosaic picture to screen before destroying context:\n");
        forceRenderMosaicToScreen();
        gRetracer.mMosaicNeedToBeFlushed = false;
    }

    //  ------------- retrace ---------------
    Context* toBeDel = gRetracer.mState.GetContext(ctx);
    DBG_LOG("CallNo[%d] retrace_eglDestroyContext(0x%x)\n", gRetracer.GetCurCallId(), ctx);
    if (toBeDel != NULL) toBeDel->release();
    gRetracer.mState.RemoveContextMap(ctx);
}

PUBLIC void retrace_eglMakeCurrent(char* src)
{
    static bool only_once_ever = true;

    // ------- ret & params definition --------
    int ret;
    int dpy;
    int draw;
    int read;
    int ctx;

    // --------- read ret & params ----------
    src = ReadFixed(src, dpy);
    src = ReadFixed(src, draw);
    src = ReadFixed(src, read);
    src = ReadFixed(src, ctx);
    src = ReadFixed(src, ret);

    //  ------------- retrace ---------------
    StateMgr& s = gRetracer.mState;
    retracer::Drawable* drawable = 0;
    if (draw)
    {
        if (gRetracer.mOptions.mForceSingleWindow && !gRetracer.mOptions.mMultiThread)
        {
            if (s.mSingleSurface)
            {
                drawable = s.mSingleSurface;
            }
            else
            {
                int width = 0;
                int height = 0;
                getSurfaceDimensions(&width, &height);
                DBG_LOG("INFO: Creating new surface because the previous one was destroyed, "
                        "and we are running in forced single window mode\n");
                if (gRetracer.mOptions.mPbufferRendering)
                {
                    EGLint const attribs[] = { EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE, EGL_NONE };
                    drawable = GLWS::instance().CreatePbufferDrawable(attribs);
                }
                else
                {
                    if (gRetracer.mOptions.eglAfrcRate != -1) {
                        compressionControlInfo compressInfo = {0};
                        if (convertToBPC(gRetracer.mOptions.eglAfrcRate, compressInfo, NULL)  == true) {
                            EGLint const attrib_list[] = { compressInfo.extension, compressInfo.rate, EGL_NONE };
                            //if (gRetracer.mOptions.mDebug > 0)
                                DBG_LOG("Enable fixed rate attrib(0x%04x, 0x%04x) on eglSurface in singleWindow\n", compressInfo.extension, compressInfo.rate);
                            drawable = GLWS::instance().CreateDrawable(width, height, 0, attrib_list);
                        }
                        else {
                            EGLint const attribs[] = { EGL_NONE };
                            drawable = GLWS::instance().CreateDrawable(width, height, 0, attribs);
                        }
                    }
                    else{
                        EGLint const attribs[] = { EGL_NONE };
                        drawable = GLWS::instance().CreateDrawable(width, height, 0, attribs);
                    }
                }
                const RetraceOptions& opt = gRetracer.mOptions;
                drawable->winWidth  = opt.mWindowWidth;
                drawable->winHeight = opt.mWindowHeight;
                if (opt.mDoOverrideResolution)
                {
                    drawable->mOverrideResRatioW = opt.mOverrideResW / (float) opt.mWindowWidth;
                    drawable->mOverrideResRatioH = opt.mOverrideResH / (float) opt.mWindowHeight;
                }
                s.InsertDrawableMap(draw, drawable);
                s.mSingleSurface = drawable;
            }
        }
        else
        {
            drawable = s.GetDrawable(draw);
        }
    }

    retracer::Context *context = gRetracer.mState.GetContext(ctx);

    if (drawable == gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getDrawable() &&
        context == gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext())
    {
        return;
    }

    if (gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getDrawable() &&
        gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()) {
        glFlush();
    }

    bool ok = GLWS::instance().MakeCurrent(drawable, context);
    if (!ok)
    {
        DBG_LOG("Warning: retrace_eglMakeCurrent failed,(0x%x) \n", eglGetError());
        return;
    }

    if (drawable && context)
    {
        RetraceOptions& o = gRetracer.mOptions;
        // overwrite the window size in the options
        if (drawable->visible)
        {
            o.mWindowWidth  = drawable->winWidth;
            o.mWindowHeight = drawable->winHeight;
            if (context->_offscrMgr)
            {
                context->_offscrMgr->framebufferTexture(o.mWindowWidth, o.mWindowHeight);
            }
        }
        if (context->_firstTimeMakeCurrent)
        {
            if (o.mDoOverrideResolution)
            {
                glViewport(0, 0, o.mOverrideResW, o.mOverrideResH);
                glScissor(0, 0, o.mOverrideResW, o.mOverrideResH);
            }
            else
            {
                glViewport(0, 0, o.mWindowWidth, o.mWindowHeight);
                glScissor(0, 0, o.mWindowWidth, o.mWindowHeight);
            }
            context->_firstTimeMakeCurrent = false;

            if (isGlesExtensionSupported("GL_KHR_debug") && gRetracer.mOptions.mDebug)
            {
                DBG_LOG("KHR_debug callback registered\n");
                _glDebugMessageCallbackKHR(callback, 0);
                _glEnable(GL_DEBUG_OUTPUT_KHR);
                // disable notifications -- they generate too much spam on some systems
                _glDebugMessageControlKHR(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION_KHR, 0, NULL, GL_FALSE);
            }
        }

        if (only_once_ever)
        {
            DBG_LOG("Vendor: %s\n", (const char*)glGetString(GL_VENDOR));
            DBG_LOG("Renderer: %s\n", (const char*)glGetString(GL_RENDERER));
            getLocalGLESVersion();

            typedef std::pair<std::string, GLenum> Se_t;
            std::vector<Se_t> stringEnumList;
            stringEnumList.push_back(Se_t("GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS     ",  GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS));
            stringEnumList.push_back(Se_t("GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS     ",  GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS));
            stringEnumList.push_back(Se_t("GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS",  GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS));
            stringEnumList.push_back(Se_t("GL_MAX_UNIFORM_BLOCK_SIZE                 ",  GL_MAX_UNIFORM_BLOCK_SIZE));
            stringEnumList.push_back(Se_t("GL_MAX_UNIFORM_BUFFER_BINDINGS            ",  GL_MAX_UNIFORM_BUFFER_BINDINGS));
            stringEnumList.push_back(Se_t("GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT ",  GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT));
            stringEnumList.push_back(Se_t("GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT        ",  GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT));

            for (std::vector<Se_t>::iterator it = stringEnumList.begin(); it != stringEnumList.end(); ++it)
            {
                GLint value = 0;
                _glGetIntegerv(it->second, &value);
                DBG_LOG("%s = %d\n", it->first.c_str(), value);
            }

            std::string version = reinterpret_cast<const char*>(_glGetString(GL_VERSION));
            MD5Digest version_md5(version);
            std::string version_md5_str = version_md5.text();

            if (gRetracer.shaderCacheVersionMD5.size() == 0)
                gRetracer.shaderCacheVersionMD5 = version_md5_str;
            if (gRetracer.shaderCacheVersionMD5 != version_md5_str)
                gRetracer.reportAndAbort("Shader cache does not match current ddk version. Remove the existing one.");

            only_once_ever = false;
        }
    }

    if (context)
    {
        gGlesFeatures.Update();
        if (gRetracer.delayedPerfmonInit) gRetracer.perfMonInit();
    }

    if (drawable && gRetracer.mOptions.mForceOffscreen)
    {
        if (!context->_offscrMgr)
        {
            RetraceOptions& o = gRetracer.mOptions;

            context->_offscrMgr = new OffscreenManager(
                o.mWindowWidth, o.mWindowHeight,
                o.mOnscrSampleW, o.mOnscrSampleH,
                o.mOnscrSampleNumX, o.mOnscrSampleNumY,
                o.mOffscreenConfig.red,
                o.mOffscreenConfig.green,
                o.mOffscreenConfig.blue,
                o.mOffscreenConfig.alpha,
                o.mOffscreenConfig.depth,
                o.mOffscreenConfig.stencil,
                o.mOffscreenConfig.msaa_samples,
                context->_profile);
            context->_offscrMgr->Init();
            context->_offscrMgr->BindOffscreenFBO(GL_FRAMEBUFFER);
        }

        if (!gRetracer.mpOffscrMgr || gRetracer.getCurTid() == gRetracer.mOptions.mRetraceTid)
            gRetracer.mpOffscrMgr = context->_offscrMgr;
    }

    gRetracer.mState.mThreadArr[gRetracer.getCurTid()].setDrawable(drawable);
    gRetracer.mState.mThreadArr[gRetracer.getCurTid()].setContext(context);
}

PUBLIC void retrace_eglCreateImageKHR(char* src)
{
    // ------- ret & params definition --------
    int dpy;
    int ctx;
    EGLenum tgt;
    int buffer;
    common::Array<int> attrib_list;
    int ret = 0;
    static bool target_not_nb_warned = false;

    // --------- read ret & params ----------
    src = ReadFixed(src, dpy);
    src = ReadFixed(src, ctx);
    src = ReadFixed(src, tgt);
    src = ReadFixed(src, buffer);
    src = Read1DArray(src, attrib_list);
    src = ReadFixed(src, ret);

    std::vector<int> attrib_list2;
    // All the attribs will be overwritten when using dma buffer
    // So we needn't check them here
    if (tgt != EGL_LINUX_DMA_BUF_EXT) {
        for (unsigned int i = 0; i < attrib_list.cnt; i += 2) {
            // 0x3148 = EGL_IMAGE_CROP_LEFT_ANDROID
            // 0x3149 = EGL_IMAGE_CROP_TOP_ANDROID
            // 0x314A = EGL_IMAGE_CROP_RIGHT_ANDROID
            // 0x314B = EGL_IMAGE_CROP_BOTTOM_ANDROID
            // All of them need EGL_ANDROID_image_crop extension
            if (attrib_list[i] >= 0x3148 && attrib_list[i] <= 0x314B) {
                if (tgt == EGL_NATIVE_BUFFER_ANDROID) {
                    if (!isEglExtensionSupported(gRetracer.mState.mEglDisplay, "EGL_ANDROID_image_crop"))
                        gRetracer.reportAndAbort("Call %d eglCreateImageKHR, attrib_list[%d] = 0x%X needs EGL_ANDROID_image_crop extension,"
                                                 " which is not supported by your GPU. Retracing would be aborted.", gRetracer.GetCurCallId(), i, attrib_list[i]);
                }
                else {  // 0x3148, 0x3149, 0x314A and 0x314B are only compatible with target=EGL_NATIVE_BUFFER_ANDROID
                    if (!target_not_nb_warned) {
                        DBG_LOG("Call %d eglCreateImageKHR, attrib_list[%d] = 0x%X is only compatible with tgt == EGL_NATIVE_BUFFER_ANDROID,"
                                " but in this call, target == 0x%X != EGL_NATIVE_BUFFER_ANDROID(0x%X)."
                                " It might be caused by a conversion of the format of this EGLImageKHR when tracing."
                                " So here we'll ignore this attrib and continue retracing. But it might bring some artifacts.\n", gRetracer.GetCurCallId(), i, attrib_list[i], tgt, EGL_NATIVE_BUFFER_ANDROID);
                        target_not_nb_warned = true;
                    }
                    continue;
                }
            }
            attrib_list2.push_back(attrib_list[i]);
            if (attrib_list[i] != EGL_NONE)
                attrib_list2.push_back(attrib_list[i + 1]);
        }
    }
    uintptr_t buffer_new = 0;
    retracer::Context* context = gRetracer.mState.GetContext(ctx);
    if (tgt == EGL_GL_TEXTURE_2D_KHR)
        buffer_new = context->getTextureMap().RValue(buffer);
    else if (tgt == EGL_NATIVE_BUFFER_ANDROID) {
        context = 0;        // EGL_NO_CONTEXT
        retracer::Context &curContext = gRetracer.getCurrentContext();
        int id = curContext.getGraphicBufferMap().RValue(buffer);
#ifdef ANDROID
        if (useGraphicBuffer)
        {
            GraphicBuffer *graphicBuffer;
            try {
                graphicBuffer = curContext.mGraphicBuffers.at(id);
            }
            catch (std::out_of_range&) {
                DBG_LOG("Cannot find the corresponding GraphicBuffer for eglCreateImageKHR in call %d. "
                    "Either a glGenGraphicBuffer missing or retracing a very old pat file.\n", gRetracer.GetCurCallId());
                goto retrace;
            }
            buffer_new = reinterpret_cast<uintptr_t>(graphicBuffer->getNativeBuffer());
        }
        else if (useHardwareBuffer)
        {
            static PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC __eglGetNativeClientBufferANDROID = NULL;

            HardwareBuffer *hardwareBuffer;
            try {
                hardwareBuffer = curContext.mHardwareBuffers.at(id);
            }
            catch (std::out_of_range&) {
                DBG_LOG("Cannot find the corresponding HardwareBuffer for eglCreateImageKHR in call %d. "
                    "Either a glGenGraphicBuffer missing or retracing a very old pat file.\n", gRetracer.GetCurCallId());
                goto retrace;
            }

            __eglGetNativeClientBufferANDROID =
                (PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC)eglGetProcAddress("eglGetNativeClientBufferANDROID");

            buffer_new = reinterpret_cast<uintptr_t>(hardwareBuffer->eglGetNativeClientBufferANDROID((void *)__eglGetNativeClientBufferANDROID));
        }
#else
        tgt = EGL_LINUX_DMA_BUF_EXT;        // need to be adjusted
        buffer_new = 0;     // NULL
        egl_image_fixture *fix;
        try {
            fix = curContext.mGraphicBuffers.at(id);
        }
        catch (std::out_of_range&) {
            DBG_LOG("Cannot find the corresponding GraphicBuffer for eglCreateImageKHR in call %d. "
                    "Either a glGenGraphicBuffer missing or retracing a very old pat file.\n", gRetracer.GetCurCallId());
            goto retrace;
        }
        attrib_list.cnt = fix->attrib_size;
        attrib_list.v = fix->attribs;
#endif
    }
    else
    {
        gRetracer.reportAndAbort("Invalid buffer type for eglCreateImageKHR");
    }

    //  ------------- retrace ---------------
retrace:
    EGLImageKHR image;

    if (tgt == EGL_LINUX_DMA_BUF_EXT)
    {
        image = GLWS::instance().createImageKHR(context, tgt, buffer_new, attrib_list);
    }
    else
    {
        image = GLWS::instance().createImageKHR(context, tgt, buffer_new, &attrib_list2[0]);
    }

    if (image == NULL) {
        DBG_LOG("eglCreateImageKHR failed to create EGLImage = 0x%x at call %u, (0x%x)\n", ret, gRetracer.GetCurCallId(), eglGetError());
    }
    gRetracer.mState.InsertEGLImageMap(ret, image);
}

PUBLIC void retrace_eglDestroyImageKHR(char* src)
{
    // ------- ret & params definition --------
    int dpy;
    int image;
    int ret = 0;

    // --------- read ret & params ----------
    src = ReadFixed(src, dpy);
    src = ReadFixed<int>(src, image);
    src = ReadFixed(src, ret);

    //  ------------- retrace ---------------
    EGLDisplay d = gRetracer.mState.mEglDisplay;
    bool found = false;
    EGLImageKHR imageNew = gRetracer.mState.GetEGLImage(image, found);
    if (found) _eglDestroyImageKHR(d, imageNew);
    else DBG_LOG("Could not find EGLImage %d (%ld) for destruction\n", image, (long)imageNew);

    gRetracer.mState.RemoveEGLImageMap(image);
}

PUBLIC void retrace_glEGLImageTargetTexture2DOES(char* src)
{
    // ------- ret & params definition --------
    EGLenum tgt;
    int image;

    // --------- read ret & params ----------
    src = ReadFixed(src, tgt);
    src = ReadFixed<int>(src, image);

    //  ------------- retrace ---------------
    bool found = false;
    EGLImageKHR imageNew = gRetracer.mState.GetEGLImage(image, found);
    if(imageNew == NULL)
    {
        if (image == 0)
        {
            DBG_LOG("glEGLImageTargetTexture2DOES tries to using an EGLImage==NULL at call %u.\n", gRetracer.GetCurCallId());
        }
        else
        {
            if (found)
            {
                DBG_LOG("The corresponding imageNew of EGLImage = 0x%x is NULL at call %u, because "
                        "the previous eglCreateImageKHR that tries to create EGLImage = 0x%x failed.\n", image, gRetracer.GetCurCallId(), image);
            }
            else
            {
                DBG_LOG("The corresponding imageNew of EGLImage = 0x%x is NULL at call %u, because "
                        "EGLImage = 0x%x was never created by any previous eglCreateImageKHR.\n", image, gRetracer.GetCurCallId(), image);
            }
        }
    }
    else
    {
        _glEGLImageTargetTexture2DOES(tgt, imageNew);
    }
}

PUBLIC void retrace_glEGLImageTargetTexStorageEXT(char* src)
{
    // ------- ret & params definition --------
    EGLenum tgt;
    int image;
    common::Array<int> attrib_list;

    // --------- read ret & params ----------
    src = ReadFixed(src, tgt);
    src = ReadFixed<int>(src, image);
    src = Read1DArray(src, attrib_list);

    //  ------------- retrace ---------------
    bool found = false;
    EGLImageKHR imageNew = gRetracer.mState.GetEGLImage(image, found);
    if (gRetracer.mOptions.eglImageAfrcRate != -1) {
        compressionControlInfo compressInfo = {0};
        getCompressionInfo(gRetracer.mOptions.eglImageAfrcRate, false, compressInfo);
        std::vector<int> new_attrib_list = AddtoAttribList(attrib_list, compressInfo.extension, compressInfo.rate, EGL_NONE);
        //if (gRetracer.mOptions.mDebug > 0)
            DBG_LOG("Enable fixed rate attrib(0x%04x, 0x%04x) on eglImage in glEGLImageTargetTexStorageEXT().\n", compressInfo.extension, compressInfo.rate);
        _glEGLImageTargetTexStorageEXT(tgt, imageNew, new_attrib_list.data());
    }
    else {
        _glEGLImageTargetTexStorageEXT(tgt, imageNew, attrib_list);
    }
}

PUBLIC void swapBuffersCommon(char* src, bool withDamage)
{
    int n_rects;
    common::Array<int> rect_list;

    gRetracer.OnFrameComplete();

    retracer::Drawable* pDrawable = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getDrawable();
    if (!pDrawable)
    {
        DBG_LOG("WARNING: pDrawable for tid=%u is 0 in swapBuffersCommon at call=%u\n", gRetracer.getCurTid(), gRetracer.GetCurCallId());
        return;
    }

    // Hmm... why is always the current drawable used as the surface to be
    // swapped? Why do we not use the incoming surface parameter? The
    // following code block tests if this is an issue. / Joakim
    {
        int dpy;
        int surface;
        int ret;

        // --------- read ret & params ----------
        src = ReadFixed(src, dpy);
        src = ReadFixed(src, surface);
        if (withDamage)
        {
            src = Read1DArray(src, rect_list);
            src = ReadFixed(src, n_rects);
        }
        src = ReadFixed(src, ret);

        retracer::Drawable* drawable = gRetracer.mState.GetDrawable(surface);
        if (drawable != pDrawable && !(gRetracer.mOptions.mForceSingleWindow) && !gRetracer.mOptions.mMultiThread)
        {
            DBG_LOG("%p != %p\n", drawable, pDrawable);
            DBG_LOG("WARNING: eglSwapBuffers assumes that it is always the "
                    "current surface that is swapped. If you the this message "
                    "please report this as a bug.\n");
        }
    }

    if (gRetracer.mOptions.mForceOffscreen)
    {
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        const unsigned int ON_SCREEN_FBO = 1;
#else
        const unsigned int ON_SCREEN_FBO = 0;
#endif

        if (gGlesFeatures.Support_GL_EXT_discard_framebuffer || gRetracer.mOptions.mApiVersion >= PROFILE_ES3)
        {
            // discard the depth and stencil buffer of the offscreen target
            gRetracer.mpOffscrMgr->BindOffscreenFBO(GL_FRAMEBUFFER);
            const GLenum DISCARD_ATTACHMENTS[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
            if (gRetracer.mOptions.mApiVersion >= PROFILE_ES3)
            {
                glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, DISCARD_ATTACHMENTS);
            }
            else
            {
                glDiscardFramebufferEXT(GL_FRAMEBUFFER, 2, DISCARD_ATTACHMENTS);
            }
        }

        gRetracer.mpOffscrMgr->OffscreenToMosaic();

        retracer::Context *pContext = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext();
        if (pContext != NULL && pDrawable != NULL) {
            int ctx = gRetracer.mState.GetCtx(pContext);
            int draw = gRetracer.mState.GetDraw(pDrawable);
            gRetracer.mpOffscrMgr->last_non_zero_ctx = ctx;
            gRetracer.mpOffscrMgr->last_non_zero_draw = draw;
            gRetracer.mpOffscrMgr->last_tid = gRetracer.getCurTid();
        }

        if (gRetracer.mpOffscrMgr->MosaicToScreenIfNeeded())
        {
            pDrawable->swapBuffers();
            gRetracer.mMosaicNeedToBeFlushed = false;
        }
        else
        {
            glFlush();
            gRetracer.mMosaicNeedToBeFlushed = true;
        }

        gRetracer.OnNewFrame();
        if (gRetracer.getCurrentContext()._current_framebuffer != ON_SCREEN_FBO)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, gRetracer.getCurrentContext()._current_framebuffer);
        }
        else
        {
            // bind the offscreen target for the next frame
            gRetracer.mpOffscrMgr->BindOffscreenFBO(GL_FRAMEBUFFER);
        }
    }
    else
    {
        if (withDamage)
        {
            pDrawable->swapBuffersWithDamage(rect_list, n_rects);
        }
        else
        {
            pDrawable->swapBuffers();
        }
        gRetracer.OnNewFrame();
    }
}

PUBLIC void retrace_eglSwapBuffers(char* src)
{
    swapBuffersCommon(src, false);
}

PUBLIC void retrace_eglSwapBuffersWithDamageKHR(char* src)
{
    swapBuffersCommon(src, true);
}

PUBLIC void retrace_eglSurfaceAttrib(char* src)
{
    // ------- ret & params definition --------
    int ret;
    int dpy;
    int surface;
    int attribute;
    int value;

    // --------- read ret & params ----------
    src = ReadFixed(src, dpy);
    src = ReadFixed(src, surface);
    src = ReadFixed(src, attribute);
    src = ReadFixed(src, value);
    src = ReadFixed(src, ret);

    //  ------------- retrace ---------------

    retracer::Drawable* d = gRetracer.mState.GetDrawable(surface);
    GLWS::instance().setAttribute(d, attribute, value);
}

PUBLIC void retrace_eglCreateSyncKHR(char* src)
{
    int dpy;
    int type;
    common::Array<int> attrib_list;
    int old_result;

    src = ReadFixed(src, dpy);
    src = ReadFixed(src, type);
    src = Read1DArray(src, attrib_list);
    src = ReadFixed(src, old_result);

    EGLDisplay d = gRetracer.mState.mEglDisplay;

    EGLSyncKHR new_result = _eglCreateSyncKHR(d, type, attrib_list);

    Context& context = gRetracer.getCurrentContext();
    context.getEGLSyncMap().LValue(old_result) = new_result;
}

PUBLIC void retrace_eglClientWaitSyncKHR(char* src)
{
    int dpy;
    int sync;
    int flags;
    uint64_t timeout;
    int32_t result;

    src = ReadFixed(src, dpy);
    src = ReadFixed(src, sync);
    src = ReadFixed(src, flags);
    src = ReadFixed(src, timeout);
    src = ReadFixed(src, result);

    Context& context = gRetracer.getCurrentContext();
    EGLSyncKHR new_sync = context.getEGLSyncMap().RValue(sync);

    EGLDisplay d = gRetracer.mState.mEglDisplay;

    EGLint new_result = _eglClientWaitSyncKHR(d, new_sync, flags, timeout);
    (void)new_result; // ignore
}

PUBLIC void retrace_eglDestroySyncKHR(char* src)
{
    int dpy;
    int sync;

    src = ReadFixed(src, dpy);
    src = ReadFixed(src, sync);

    Context& context = gRetracer.getCurrentContext();
    EGLSyncKHR new_sync = context.getEGLSyncMap().RValue(sync);

    EGLDisplay d = gRetracer.mState.mEglDisplay;

    EGLint new_result = _eglDestroySyncKHR(d, new_sync);
    (void)new_result; // ignore
}

PUBLIC void retrace_eglSignalSyncKHR(char* src)
{
    int dpy;
    int sync;
    int mode;

    src = ReadFixed(src, dpy);
    src = ReadFixed(src, sync);
    src = ReadFixed(src, mode);

    Context& context = gRetracer.getCurrentContext();
    EGLSyncKHR new_sync = context.getEGLSyncMap().RValue(sync);

    EGLDisplay d = gRetracer.mState.mEglDisplay;

    EGLint new_result = _eglSignalSyncKHR(d, new_sync, mode);
    (void)new_result; // ignore
}

PUBLIC void retrace_eglSetDamageRegionKHR(char* _src)
{
    int dpy;
    int surface;
    Array<int> rects;
    int n_rects;
    int old_ret;

    _src = ReadFixed<int>(_src, dpy);
    _src = ReadFixed<int>(_src, surface);
    _src = Read1DArray(_src, rects);
    _src = ReadFixed<int>(_src, n_rects);
    _src = ReadFixed<int>(_src, old_ret);
    if (old_ret == EGL_FALSE) return; // failed in original, so fail here as well

    gRetracer.mState.GetDrawable(surface)->setDamage(rects.v, n_rects);
}

PUBLIC void retrace_eglQuerySurface(char* _src)
{
    int dpy;
    int surface;
    int attrib;
    unsigned int valid;
    int old_value;
    int new_value;
    int old_ret;

    _src = ReadFixed(_src, dpy);
    _src = ReadFixed(_src, surface);
    _src = ReadFixed(_src, attrib);
    _src = ReadFixed(_src, valid);
    if (valid == 1)
        _src = ReadFixed(_src, old_value);
    _src = ReadFixed(_src, old_ret);

    if (old_ret == EGL_FALSE){
        DBG_LOG("eglQuerySurface return EGL_FALSE in original app.\n");
    }

    retracer::Drawable* drawable = gRetracer.mState.GetDrawable(surface);
    if (drawable) {
        drawable->querySurface(attrib, &new_value);
    } else {
        DBG_LOG("WARNING: surface %d has been destroyed and QuerySurface(0x%x) would be ignored.\n", surface, attrib);
    }
}

const common::EntryMap retracer::egl_callbacks = {
    {"eglCreateSyncKHR", std::make_pair((void*)retrace_eglCreateSyncKHR, false)},
    {"eglClientWaitSyncKHR", std::make_pair((void*)retrace_eglClientWaitSyncKHR, false)},
    {"eglDestroySyncKHR", std::make_pair((void*)retrace_eglDestroySyncKHR, false)},
    {"eglSignalSyncKHR", std::make_pair((void*)retrace_eglSignalSyncKHR, false)},
    {"eglGetSyncAttribKHR", std::make_pair((void*)ignore, true)},
    {"eglGetError", std::make_pair((void*)ignore, true)},
    {"eglGetDisplay", std::make_pair((void*)ignore, true)},
    {"eglInitialize", std::make_pair((void*)ignore, true)},
    {"eglTerminate", std::make_pair((void*)ignore, true)},
    {"eglQueryString", std::make_pair((void*)ignore, true)},
    {"eglGetConfigs", std::make_pair((void*)ignore, true)},
    {"eglChooseConfig", std::make_pair((void*)ignore, true)},
    {"eglGetConfigAttrib", std::make_pair((void*)ignore, true)},
    {"eglSetDamageRegionKHR", std::make_pair((void*)retrace_eglSetDamageRegionKHR, false)},
    {"eglCreateWindowSurface", std::make_pair((void*)retrace_eglCreateWindowSurface, false)},
    {"eglCreateWindowSurface2", std::make_pair((void*)retrace_eglCreateWindowSurface2, false)},
    {"eglCreatePlatformWindowSurface", std::make_pair((void*)retrace_eglCreateWindowSurface, false)},
    {"eglCreatePbufferSurface", std::make_pair((void*)retrace_eglCreatePbufferSurface, false)},
    //{"eglCreatePixmapSurface", std::make_pair((void*)ignore, true)},
    {"eglDestroySurface", std::make_pair((void*)retrace_eglDestroySurface, false)},
    {"eglQuerySurface", std::make_pair((void*)retrace_eglQuerySurface, false)},
    {"eglBindAPI", std::make_pair((void*)retrace_eglBindAPI, false)},
    {"eglQueryAPI", std::make_pair((void*)ignore, true)},
    //{"eglWaitClient", std::make_pair((void*)ignore, true)},
    //{"eglReleaseThread", std::make_pair((void*)ignore, true)},
    //{"eglCreatePbufferFromClientBuffer", std::make_pair((void*)ignore, true)},
    {"eglSurfaceAttrib", std::make_pair((void*)retrace_eglSurfaceAttrib, false)},
    //{"eglBindTexImage", std::make_pair((void*)ignore, true)},
    //{"eglReleaseTexImage", std::make_pair((void*)ignore, true)},
    {"eglSwapInterval", std::make_pair((void*)ignore, true)},
    {"eglCreateContext", std::make_pair((void*)retrace_eglCreateContext, false)},
    {"eglDestroyContext", std::make_pair((void*)retrace_eglDestroyContext, false)},
    {"eglMakeCurrent", std::make_pair((void*)retrace_eglMakeCurrent, false)},
    {"eglGetCurrentContext", std::make_pair((void*)ignore, true)},
    {"eglGetCurrentSurface", std::make_pair((void*)ignore, true)},
    {"eglGetCurrentDisplay", std::make_pair((void*)ignore, true)},
    {"eglQueryContext", std::make_pair((void*)ignore, true)},
    {"eglWaitGL", std::make_pair((void*)ignore, true)},
    {"eglWaitNative", std::make_pair((void*)ignore, true)},
    {"eglSwapBuffers", std::make_pair((void*)retrace_eglSwapBuffers, false)},
    {"eglSwapBuffersWithDamageKHR", std::make_pair((void*)retrace_eglSwapBuffersWithDamageKHR, false)},
    //{"eglCopyBuffers", std::make_pair((void*)ignore, true)},
    {"eglGetProcAddress", std::make_pair((void*)ignore, true)},
    {"eglCreateImageKHR", std::make_pair((void*)retrace_eglCreateImageKHR, false)},
    {"eglDestroyImageKHR", std::make_pair((void*)retrace_eglDestroyImageKHR, false)},
    {"glEGLImageTargetTexture2DOES", std::make_pair((void*)retrace_glEGLImageTargetTexture2DOES, false)},
    {"glEGLImageTargetTexStorageEXT", std::make_pair((void*)retrace_glEGLImageTargetTexStorageEXT, false)},
};
