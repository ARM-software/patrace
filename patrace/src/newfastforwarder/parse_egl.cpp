#include <newfastforwarder/parser.hpp>
#include <map>

using namespace common;
using namespace retracer;
using namespace std;

static void retrace_eglSwapBuffers(char* src)
{
    gRetracer.OnNewFrame();
}

static void retrace_eglSwapBuffersWithDamageKHR(char* src)
{
    gRetracer.OnNewFrame();
}

static void retrace_eglDestroySurface(char* src)
{
    //gRetracer.OnFrameComplete();
    // ------- ret & params definition --------
    int ret;
    int dpy;
    int surface;

    // --------- read ret & params ----------
    src = ReadFixed(src, dpy);
    src = ReadFixed(src, surface);
    src = ReadFixed(src, ret);

    //  ------------- retrace --------------
    //DBG_LOG("CallNo[%d] retrace_eglDestroySurface()\n", gRetracer.GetCurCallId());
    gRetracer.curContextState->curEglState.ff_eglDestroySurface(gRetracer.mCurCallNo);
    gRetracer.callNoList.insert(gRetracer.mCurCallNo);
}


static void retrace_eglMakeCurrent(char* src)
{
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

    //gRetracer.curContextState->curEglState.ff_eglMakeCurrent(gRetracer.mCurCallNo);
//if(ctx==-1173466248)ctx=-923802168;
//if(ctx==-1047586656||ctx==-1022366992)ctx=-1119160608;
    int ctx_share = 0;
    map<int, int>::iterator it_ctx = gRetracer.share_context_map.find(ctx);
    if(it_ctx != gRetracer.share_context_map.end())
    {
        if(it_ctx->second != 1)
        {
            ctx_share = it_ctx->second;
            ctx = it_ctx->second;
        }
        else
        {
            ctx_share = 1;
        }
    }

    if(gRetracer.defaultContextJudge == false)
    {
        gRetracer.multiThreadContextState.insert(pair<int, newfastforwad::contextState>(ctx, gRetracer.defaultContextState));
        map<int, newfastforwad::contextState>::iterator it0 = gRetracer.multiThreadContextState.find(ctx);
        gRetracer.curContextState = &(it0->second);
        gRetracer.defaultContextJudge = true;
    }
    else
    {
        map<int, newfastforwad::contextState>::iterator it = gRetracer.multiThreadContextState.find(ctx);
        if(it != gRetracer.multiThreadContextState.end())
        {
            gRetracer.curContextState = &(it->second);
        }
        else
        {
            newfastforwad::contextState  newContext(gRetracer.glClientSideBuffDataNoListForShare);
            gRetracer.multiThreadContextState.insert(pair<int, newfastforwad::contextState>(ctx, newContext));
            it = gRetracer.multiThreadContextState.find(ctx);
            gRetracer.curContextState = &(it->second);
        }
    }

    gRetracer.curContextState->curEglState.ff_eglMakeCurrent(gRetracer.mCurCallNo);
    gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    if(ctx_share != 0)
    {
        //gRetracer.curContextState->share_context = ctx_share;
        //gRetracer.curContextState->curTextureState.share_ctx = ctx_share;
        gRetracer.curContextState->curTextureState.readContext(ctx);
    }
}

static void retrace_eglCreateWindowSurface(char* src)
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

    gRetracer.curContextState->curEglState.ff_eglCreateWindowSurface(gRetracer.mCurCallNo);
    gRetracer.callNoList.insert(gRetracer.mCurCallNo);
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
    //    int surfWidth;
    //    int surfHeight;
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
    src = ReadFixed(src, ret);

    gRetracer.curContextState->curEglState.ff_eglCreateWindowSurface(gRetracer.mCurCallNo);
    gRetracer.callNoList.insert(gRetracer.mCurCallNo);
}

static void retrace_eglCreateContext(char* src)
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
    gRetracer.curContextState->curEglState.ff_eglCreateContext(gRetracer.mCurCallNo);
    gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    if(share_context != 0)
    {
        gRetracer.share_context_map.insert(pair<int, int>(share_context, 1));
        gRetracer.share_context_map.insert(pair<int, int>(ret, share_context));
    }
}


const common::EntryMap retracer::egl_callbacks = {
    {"eglCreateSyncKHR", (void*)ignore},//fast..(void*)retrace_eglCreateSyncKHR},
    {"eglClientWaitSyncKHR", (void*)ignore},//fast..(void*)retrace_eglClientWaitSyncKHR},
    {"eglDestroySyncKHR", (void*)ignore},//fast..(void*)retrace_eglDestroySyncKHR},
    {"eglSignalSyncKHR", (void*)ignore},//fast..(void*)retrace_eglSignalSyncKHR},
    {"eglGetSyncAttribKHR", (void*)ignore},
    {"eglGetError", (void*)ignore},
    {"eglGetDisplay", (void*)ignore},
    {"eglInitialize", (void*)ignore},
    {"eglTerminate", (void*)ignore},
    {"eglQueryString", (void*)ignore},
    {"eglGetConfigs", (void*)ignore},
    {"eglChooseConfig", (void*)ignore},
    {"eglGetConfigAttrib", (void*)ignore},
    {"eglCreateWindowSurface", (void*)retrace_eglCreateWindowSurface},
    {"eglCreateWindowSurface2", (void*)retrace_eglCreateWindowSurface2},
    {"eglCreatePbufferSurface", (void*)ignore},//fast..(void*)retrace_eglCreatePbufferSurface},
    //{"eglCreatePixmapSurface", (void*)ignore},
    {"eglDestroySurface", (void*)retrace_eglDestroySurface},
    {"eglQuerySurface", (void*)ignore},
    {"eglBindAPI", (void*)ignore},//fast..(void*)retrace_eglBindAPI},
    {"eglQueryAPI", (void*)ignore},
    //{"eglWaitClient", (void*)ignore},
    //{"eglReleaseThread", (void*)ignore},
    //{"eglCreatePbufferFromClientBuffer", (void*)ignore},
    {"eglSurfaceAttrib", (void*)ignore},//fast..(void*)retrace_eglSurfaceAttrib},
    //{"eglBindTexImage", (void*)ignore},
    //{"eglReleaseTexImage", (void*)ignore},
    {"eglSwapInterval", (void*)ignore},
    {"eglCreateContext", (void*)retrace_eglCreateContext},
    {"eglDestroyContext", (void*)ignore},//fast..(void*)retrace_eglDestroyContext},
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
    {"eglCreateImageKHR", (void*)ignore},//fast..(void*)retrace_eglCreateImageKHR},
    {"eglDestroyImageKHR", (void*)ignore},//fast..(void*)retrace_eglDestroyImageKHR},
    {"glEGLImageTargetTexture2DOES", (void*)ignore},//fast..(void*)retrace_glEGLImageTargetTexture2DOES},
};
