#include <retracer/glws.hpp>
#include <retracer/retracer.hpp>


#include <dispatch/eglproc_auto.hpp>

#include <common/os.hpp>

#include <cmath>
#include <WINDOWS.h>

using namespace os;
using namespace common;

namespace retracer
{

static EGLDisplay       gEglDisplay = EGL_NO_DISPLAY;
static int              gScreen = 0;
static EGLConfig        gConfig = 0;

EglPbufferDrawable::EglPbufferDrawable(EGLint const* attribList)
    : PbufferDrawable(attribList)
    , mSurface(eglCreatePbufferSurface(gEglDisplay, gConfig, attribList))
{
    if (mSurface == EGL_NO_SURFACE)
    {
        gRetracer.reportAndAbort("Error creating pbuffer surface! (0x%04x)", eglGetError());
    }
}

EglPbufferDrawable::~EglPbufferDrawable()
{
    eglDestroySurface(gEglDisplay, mSurface);
}

static LRESULT CALLBACK
    WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MINMAXINFO *pMMI;
    switch (uMsg) {
    case WM_GETMINMAXINFO:
        // Allow to create a window bigger than the desktop
        pMMI = (MINMAXINFO *)lParam;
        pMMI->ptMaxSize.x = 60000;
        pMMI->ptMaxSize.y = 60000;
        pMMI->ptMaxTrackSize.x = 60000;
        pMMI->ptMaxTrackSize.y = 60000;
        break;
    default:
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

class EglDrawable : public Drawable
{
public:
    HWND hWnd;
    HDC hDC;

    EGLSurface mSurface;
    EGLint api;

    EglDrawable(int w, int h, EGLint const* attribList):
        Drawable(w, h), hWnd(NULL), api(EGL_OPENGL_ES_API)
    {
        static bool first = true;

        if (first)
        {
            WNDCLASS wc;
            memset(&wc, 0, sizeof wc);
            wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
            wc.lpfnWndProc = WndProc;
            wc.lpszClassName = "eglretrace";
            wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
            RegisterClass(&wc);
            first = FALSE;
        }

        DWORD dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW;
        DWORD dwExStyle = 0;

        RECT rect = { 0, 0, w, h};
        AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);
        hWnd = CreateWindowEx(dwExStyle,
            "eglretrace", /* wc.lpszClassName */
            NULL,
            dwStyle,
            0, /* x */
            0, /* y */
            rect.right - rect.left, /* width */
            rect.bottom - rect.top, /* height */
            NULL,
            NULL,
            NULL,
            NULL);
        hDC = GetDC(hWnd);

        mSurface = eglCreateWindowSurface(gEglDisplay, gConfig, (EGLNativeWindowType)hWnd, attribList);
        if (mSurface != EGL_NO_SURFACE)
        {
            show();
        }
        else
        {
            gRetracer.reportAndAbort("Error creating window surface! (0x%04x)", eglGetError());
        }
    }

    virtual ~EglDrawable() {
        eglDestroySurface(gEglDisplay, mSurface);
        eglWaitClient();
        ReleaseDC(hWnd, hDC);
        DestroyWindow(hWnd);
    }

    virtual void resize(int w, int h)
    {
        if (w == width && h == height)
            return;

        eglWaitClient();

        RECT rClient, rWindow;
        GetClientRect(hWnd, &rClient);
        GetWindowRect(hWnd, &rWindow);
        int win_w = w + (rWindow.right  - rWindow.left) - rClient.right;
        int win_h = h + (rWindow.bottom - rWindow.top)  - rClient.bottom;
        SetWindowPos(hWnd, NULL, rWindow.left, rWindow.top, win_w, win_h, SWP_NOMOVE);

        Drawable::resize(w, h);
    }

    virtual void show(void) {
        if (visible) {
            return;
        }

        eglWaitClient();

        ShowWindow(hWnd, SW_SHOW);

        Drawable::show();
    }

    virtual void swapBuffers(void) {
        eglSwapBuffers(gEglDisplay, mSurface);

        // Drain message queue to prevent window from being considered
        // non-responsive
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    virtual void swapBuffersWithDamage(int *rects, int n_rects) {
        static bool notSupported = false;

        if (notSupported)
        {
            swapBuffers();
            return;
        }

        if (!eglSwapBuffersWithDamageKHR(gEglDisplay, mSurface, rects, n_rects))
        {
            DBG_LOG("WARNING: eglSwapBuffersWithDamageKHR() may not be suppported, fallback to eglSwapBuffers()\n");
            swapBuffers();
            notSupported = true;
        }
    }

    virtual void processStepEvent()
    {
        while (1)
        {
            if (GetAsyncKeyState(VK_F1))
            {
                if (!gRetracer.RetraceForward(1, 0))
                    return;
            }
            else if (GetAsyncKeyState(VK_F2))
            {
                if (!gRetracer.RetraceForward(10, 0))
                    return;
            }
            else if (GetAsyncKeyState(VK_F3))
            {
                if (!gRetracer.RetraceForward(100, 0))
                    return;
            }
            else if (GetAsyncKeyState(VK_F4))
            {
                if (!gRetracer.RetraceForward(1000, 0))
                    return;
            }
            else if (GetAsyncKeyState(VK_F5))
            {
                if (!gRetracer.RetraceForward(0, 1))
                    return;
            }
            else if (GetAsyncKeyState(VK_F6))
            {
                if (!gRetracer.RetraceForward(0, 10))
                    return;
            }
            else if (GetAsyncKeyState(VK_F7))
            {
                if (!gRetracer.RetraceForward(0, 100))
                    return;
            }
            else if (GetAsyncKeyState(VK_F8))
            {
                if (!gRetracer.RetraceForward(0, 1000))
                    return;
            }
        }
    }
};


class EglContext : public Context
{
public:
    EGLContext mContext;

    EglContext(Profile prof, EGLContext ctx) :
        Context(prof),
        mContext(ctx)
    {}

    ~EglContext() {
        eglDestroyContext(gEglDisplay, mContext);
    }
};

void GLWS::Init(Profile /*profile*/)
{
    HDC hDisplay = EGL_DEFAULT_DISPLAY;
    gEglDisplay = eglGetDisplay((EGLNativeDisplayType)hDisplay);
    if (gEglDisplay == EGL_NO_DISPLAY)
    {
        gRetracer.reportAndAbort("Unable to get EGL display: %d", eglGetError());
    }
    gRetracer.mState.mEglDisplay = gEglDisplay;

    EGLint major, minor;
    if (!eglInitialize(gEglDisplay, &major, &minor))
    {
        gRetracer.reportAndAbort("Unable to initialize EGL display: %d", eglGetError());
    }

    // When starting, we dont care about EGL parameters color, alpha, stencil, depth, because all rendering is done to an offscreen quad.
    // see offscreen manager for offscreen rendering bitdepths (color,alpha,stencil,depth)
    const EGLint attribs[] = {
        EGL_SAMPLE_BUFFERS, gRetracer.mOptions.mMSAA_Samples>0 ? 1 : 0,
        EGL_SAMPLES, gRetracer.mOptions.mMSAA_Samples>0 ? gRetracer.mOptions.mMSAA_Samples : 0, // ensure number samples is valid
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 1,
        EGL_DEPTH_SIZE, 1,
        EGL_STENCIL_SIZE, 1,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,//EGL_OPENGL_ES_BIT | GLES3 emulator doesn't support GL ES 1 context
        EGL_NONE
    };

    EGLint configCnt;
    // When 3rd argument is NULL, no configs will be returned, instead we get the number of compatible configs
    EGLint result = eglChooseConfig(gEglDisplay, attribs, NULL, 0, &configCnt);

    if (result == EGL_FALSE || configCnt == 0)
    {
        gRetracer.reportAndAbort("eglChooseConfig failed: %d", eglGetError());
    }

    if (eglChooseConfig(gEglDisplay, attribs, &gConfig, 1, &configCnt) == EGL_FALSE || configCnt != 1)
    {
        gRetracer.reportAndAbort("eglChooseConfig failed: %d", eglGetError());
    }

    EGLint msaa_sample_buffers = -1, msaa_samples = -1, red = -1, green = -1, blue = -1, alpha = -1, depth = -1, stencil = -1;
    eglGetConfigAttrib(gEglDisplay, gConfig, EGL_SAMPLE_BUFFERS, &msaa_sample_buffers);
    eglGetConfigAttrib(gEglDisplay, gConfig, EGL_SAMPLES,        &msaa_samples);
    eglGetConfigAttrib(gEglDisplay, gConfig, EGL_RED_SIZE,       &red);
    eglGetConfigAttrib(gEglDisplay, gConfig, EGL_GREEN_SIZE,     &green);
    eglGetConfigAttrib(gEglDisplay, gConfig, EGL_BLUE_SIZE,      &blue);
    eglGetConfigAttrib(gEglDisplay, gConfig, EGL_ALPHA_SIZE,     &alpha);
    eglGetConfigAttrib(gEglDisplay, gConfig, EGL_DEPTH_SIZE,     &depth);
    eglGetConfigAttrib(gEglDisplay, gConfig, EGL_STENCIL_SIZE,   &stencil);

    DBG_LOG("EGL_SAMPLE_BUFFERS: %d\n", msaa_sample_buffers);
    DBG_LOG("EGL_SAMPLES:        %d\n", msaa_samples);
    DBG_LOG("EGL_RED_SIZE:       %d\n", red);
    DBG_LOG("EGL_GREEN_SIZE:     %d\n", green);
    DBG_LOG("EGL_BLUE_SIZE:      %d\n", blue);
    DBG_LOG("EGL_ALPHA_SIZE:     %d\n", alpha);
    DBG_LOG("EGL_DEPTH_SIZE:     %d\n", depth);
    DBG_LOG("EGL_STENCIL_SIZE:   %d\n", stencil);
}

void GLWS::Cleanup()
{
    if (gEglDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(gEglDisplay, NULL, NULL, NULL);
        eglTerminate(gEglDisplay);
        gEglDisplay = EGL_NO_DISPLAY;
    }
}

Drawable* GLWS::CreateDrawable(int width, int height, int /*win*/, EGLint const* attribList)
{
    return new EglDrawable(width, height, attribList);
}

Context* GLWS::CreateContext(Context* shareContext, Profile profile)
{
    EGLint attribs[] = {EGL_CONTEXT_CLIENT_VERSION,profile, EGL_NONE};

    EGLContext eglShareContext = EGL_NO_CONTEXT;
    if (shareContext)
        eglShareContext = static_cast<EglContext*>(shareContext)->mContext;

    EGLContext newCtx = eglCreateContext(
        gEglDisplay, gConfig, eglShareContext, attribs);
    if (!newCtx)
        return NULL;

    return new EglContext(profile, newCtx);
}

bool GLWS::MakeCurrent(Drawable* drawable, Context* context)
{
    EGLSurface eglSurface = EGL_NO_SURFACE;

    if (dynamic_cast<EglDrawable*>(drawable))
    {
        eglSurface = static_cast<EglDrawable*>(drawable)->mSurface;
    }
    else if (dynamic_cast<EglPbufferDrawable*>(drawable))
    {
        eglSurface = static_cast<EglPbufferDrawable*>(drawable)->mSurface;
    }

    EGLContext eglContext = EGL_NO_CONTEXT;

    if (context)
    {
        eglContext = static_cast<EglContext *>(context)->mContext;
    }

    EGLBoolean ok = eglMakeCurrent(gEglDisplay, eglSurface, eglSurface, eglContext);

    return ok == EGL_TRUE;
}

Drawable* GLWS::CreatePbufferDrawable(EGLint const* attribList)
{
    return new EglPbufferDrawable(attribList);
}

}
