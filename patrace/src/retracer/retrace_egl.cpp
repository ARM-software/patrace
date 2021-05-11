#include <retracer/retracer.hpp>
#include <retracer/glws.hpp>
#include <retracer/retrace_api.hpp>
#include <retracer/forceoffscreen/offscrmgr.h>
#include <common/gl_extension_supported.hpp>
#include <common/trace_limits.hpp>
#include <helper/eglstring.hpp>
#include "../dispatch/eglproc_auto.hpp"

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

void retrace_eglCreateWindowSurface(char* src)
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

    DBG_LOG("Creating drawable for surface %d: w=%d, h=%d...\n", ret, width, height);
    retracer::Drawable* d;
    if (gRetracer.mOptions.mPbufferRendering || (gRetracer.mOptions.mSingleSurface != -1 && gRetracer.mOptions.mSingleSurface != gRetracer.mSurfaceCount))
    {
        EGLint const attribs[] = { EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE, EGL_NONE };
        d = GLWS::instance().CreatePbufferDrawable(attribs);
    }
    else
    {
        d = GLWS::instance().CreateDrawable(width, height, win, attrib_list);
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

static void retrace_eglCreateWindowSurface2(char* src)
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

    DBG_LOG("Creating drawable for surface %d: x=%d, y=%d, w=%d, h=%d...\n", ret, x, y, surfWidth, surfHeight);
    retracer::Drawable* d;
    if (gRetracer.mOptions.mPbufferRendering || (gRetracer.mOptions.mSingleSurface != -1 && gRetracer.mOptions.mSingleSurface != gRetracer.mSurfaceCount))
    {
        EGLint const attribs[] = { EGL_WIDTH, surfWidth, EGL_HEIGHT, surfHeight, EGL_NONE, EGL_NONE };
        d = GLWS::instance().CreatePbufferDrawable(attribs);
    }
    else
    {
        d = GLWS::instance().CreateDrawable(surfWidth, surfHeight, win, attrib_list);
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

static void retrace_eglCreatePbufferSurface(char* src)
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

static void retrace_eglDestroySurface(char* src)
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

static void retrace_eglBindAPI(char* src)
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

void retrace_eglCreateContext(char* src)
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

static void retrace_eglDestroyContext(char* src)
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

void retrace_eglMakeCurrent(char* src)
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
                    EGLint const attribs[] = { EGL_NONE };
                    drawable = GLWS::instance().CreateDrawable(width, height, 0, attribs);
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
            DBG_LOG("Version: %s\n", (const char*)glGetString(GL_VERSION));

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

static void retrace_eglCreateImageKHR(char* src)
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

static void retrace_eglDestroyImageKHR(char* src)
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
    gRetracer.mState.RemoveEGLImageMap(image);
}

static void retrace_glEGLImageTargetTexture2DOES(char* src)
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

static void swapBuffersCommon(char* src, bool withDamage)
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

static void retrace_eglSwapBuffers(char* src)
{
    swapBuffersCommon(src, false);
}

static void retrace_eglSwapBuffersWithDamageKHR(char* src)
{
    swapBuffersCommon(src, true);
}

static void retrace_eglSurfaceAttrib(char* src)
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

static void retrace_eglCreateSyncKHR(char* src)
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

static void retrace_eglClientWaitSyncKHR(char* src)
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

static void retrace_eglDestroySyncKHR(char* src)
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

static void retrace_eglSignalSyncKHR(char* src)
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

static void retrace_eglSetDamageRegionKHR(char* _src)
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

static void retrace_eglQuerySurface(char* _src)
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
    {"eglCreateSyncKHR", (void*)retrace_eglCreateSyncKHR},
    {"eglClientWaitSyncKHR", (void*)retrace_eglClientWaitSyncKHR},
    {"eglDestroySyncKHR", (void*)retrace_eglDestroySyncKHR},
    {"eglSignalSyncKHR", (void*)retrace_eglSignalSyncKHR},
    {"eglGetSyncAttribKHR", (void*)ignore},
    {"eglGetError", (void*)ignore},
    {"eglGetDisplay", (void*)ignore},
    {"eglInitialize", (void*)ignore},
    {"eglTerminate", (void*)ignore},
    {"eglQueryString", (void*)ignore},
    {"eglGetConfigs", (void*)ignore},
    {"eglChooseConfig", (void*)ignore},
    {"eglGetConfigAttrib", (void*)ignore},
    {"eglSetDamageRegionKHR", (void*)retrace_eglSetDamageRegionKHR},
    {"eglCreateWindowSurface", (void*)retrace_eglCreateWindowSurface},
    {"eglCreateWindowSurface2", (void*)retrace_eglCreateWindowSurface2},
    {"eglCreatePbufferSurface", (void*)retrace_eglCreatePbufferSurface},
    //{"eglCreatePixmapSurface", (void*)ignore},
    {"eglDestroySurface", (void*)retrace_eglDestroySurface},
    {"eglQuerySurface", (void*)retrace_eglQuerySurface},
    {"eglBindAPI", (void*)retrace_eglBindAPI},
    {"eglQueryAPI", (void*)ignore},
    //{"eglWaitClient", (void*)ignore},
    //{"eglReleaseThread", (void*)ignore},
    //{"eglCreatePbufferFromClientBuffer", (void*)ignore},
    {"eglSurfaceAttrib", (void*)retrace_eglSurfaceAttrib},
    //{"eglBindTexImage", (void*)ignore},
    //{"eglReleaseTexImage", (void*)ignore},
    {"eglSwapInterval", (void*)ignore},
    {"eglCreateContext", (void*)retrace_eglCreateContext},
    {"eglDestroyContext", (void*)retrace_eglDestroyContext},
    {"eglMakeCurrent", (void*)retrace_eglMakeCurrent},
    {"eglGetCurrentContext", (void*)ignore},
    {"eglGetCurrentSurface", (void*)ignore},
    {"eglGetCurrentDisplay", (void*)ignore},
    {"eglQueryContext", (void*)ignore},
    {"eglWaitGL", (void*)ignore},
    {"eglWaitNative", (void*)ignore},
    {"eglSwapBuffers", (void*)retrace_eglSwapBuffers},
    {"eglSwapBuffersWithDamageKHR", (void*)retrace_eglSwapBuffersWithDamageKHR},
    //{"eglCopyBuffers", (void*)ignore},
    {"eglGetProcAddress", (void*)ignore},
    {"eglCreateImageKHR", (void*)retrace_eglCreateImageKHR},
    {"eglDestroyImageKHR", (void*)retrace_eglDestroyImageKHR},
    {"glEGLImageTargetTexture2DOES", (void*)retrace_glEGLImageTargetTexture2DOES},
};
