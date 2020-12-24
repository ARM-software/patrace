#include "retracer/glws_egl_x11.hpp"
#include "retracer/retracer.hpp"

#include "dispatch/eglproc_auto.hpp"

#include <cmath>
#include <sstream>

namespace retracer
{

class X11Window : public NativeWindow
{
public:
    X11Window(int width, int height, const std::string& title, EGLNativeDisplayType display, EGLint eglNativeVisualId)
        : NativeWindow(width, height, title)
          , mDisplay(display)
    {
        Window root = RootWindow(mDisplay, DefaultScreen(mDisplay));

        XVisualInfo tempVI;
        tempVI.visualid = eglNativeVisualId;
        int visualCnt = 0;
        XVisualInfo* visualInfo = XGetVisualInfo(mDisplay, VisualIDMask, &tempVI, &visualCnt);

        XSetWindowAttributes attr;
        attr.background_pixel = 0;
        attr.border_pixel = 0;
        attr.colormap = XCreateColormap(mDisplay, root, visualInfo->visual, AllocNone);
        attr.event_mask = StructureNotifyMask;

        unsigned long mask = 0;
        mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

        int x = 0;
        int y = 0;

        mHandle = XCreateWindow(mDisplay, root, x, y, getWidth(), getHeight(), 0, visualInfo->depth, InputOutput, visualInfo->visual, mask, &attr);

        XSizeHints sizehints;
        sizehints.x = x;
        sizehints.y = y;
        sizehints.width  = getWidth();
        sizehints.height = getHeight();
        sizehints.flags = USSize | USPosition;
        XSetNormalHints(mDisplay, mHandle, &sizehints);
        XSelectInput(mDisplay, mHandle, StructureNotifyMask | KeyPressMask | ButtonPressMask);

        XSetStandardProperties(mDisplay, mHandle, title.c_str(), title.c_str(), None, (char **)NULL, 0, &sizehints);

        XClassHint classHint;
        classHint.res_name = const_cast<char*>("paretrace");
        classHint.res_class = const_cast<char*>("paretrace");
        XSetClassHint(mDisplay, mHandle, &classHint);

        eglWaitNative(EGL_CORE_NATIVE_ENGINE);
    }

    ~X11Window()
    {
    }

    virtual void show()
    {
        if (mVisible)
        {
            return;
        }

        eglWaitClient();

        XMapWindow(mDisplay, mHandle);
        waitForEvent(MapNotify);

        eglWaitNative(EGL_CORE_NATIVE_ENGINE);

        NativeWindow::show();
    }

    bool resize(int w, int h)
    {
        if (NativeWindow::resize(w, h))
        {
            eglWaitClient();

            XResizeWindow(mDisplay, mHandle, w, h);
            waitForEvent(ConfigureNotify);
            eglWaitNative(EGL_CORE_NATIVE_ENGINE);
            return true;
        }

        return false;
    }

private:
    void waitForEvent(int type)
    {
        XEvent event;
        do
        {
            // XWindowEvent is a blocking call, returns first when desired event is recieved.
            XWindowEvent(mDisplay, mHandle, StructureNotifyMask, &event);
        } while (event.type != type);
    }

    EGLNativeDisplayType mDisplay;
};


GlwsEglX11::GlwsEglX11()
    : GlwsEgl()
{
}

GlwsEglX11::~GlwsEglX11()
{
}

void GlwsEglX11::processStepEvent()
{
    XEvent event;
    while (1)
    {
        if (XCheckTypedEvent(mEglNativeDisplay, KeyPress, &event))
        {
            const KeySym ks = XLookupKeysym(&event.xkey, 0);
            if (ks >= XK_F1 && ks <= XK_F4)
            {
                const unsigned int forwardNum = static_cast<unsigned int>(std::pow(10, ks - XK_F1));
                gRetracer.frameBudget += forwardNum;
                return;
            }
            else if (ks >= XK_F5 && ks <= XK_F8)
            {
                const unsigned int drawNum = static_cast<unsigned int>(std::pow(10, ks - XK_F5));
                gRetracer.drawBudget += drawNum;
                return;
            }
            else if (ks == XK_F10)
            {
                gRetracer.mFinish = true;
                return;
            }
        }
    }
}

EGLNativeDisplayType GlwsEglX11::getNativeDisplay()
{
    if (gRetracer.mOptions.mPbufferRendering)
    {
        return EGL_DEFAULT_DISPLAY;
    }
    EGLNativeDisplayType display = XOpenDisplay(NULL);
    if (!display)
    {
        gRetracer.reportAndAbort("Unable to open display %s", XDisplayName(NULL));
    }
    return display;
}

void GlwsEglX11::releaseNativeDisplay(EGLNativeDisplayType display)
{
    if (!gRetracer.mOptions.mPbufferRendering)
    {
        XCloseDisplay(display);
    }
}

Drawable* GlwsEglX11::CreateDrawable(int width, int height, int win, EGLint const* attribList)
{
    Drawable* handle = NULL;

    WinNameToNativeWindowMap_t::iterator it = gWinNameToNativeWindowMap.find(win);

    NativeWindow* window = 0;
    if (it != gWinNameToNativeWindowMap.end())  // if found
    {
        window = it->second;
        if (window->getWidth() != width ||
            window->getHeight() != height)
        {
            window->resize(width, height);
        }
    }
    else if (!gRetracer.mOptions.mPbufferRendering)
    {
        std::stringstream title;
        title << win << " - eglretrace";
        // TODO: Delete
        window = new X11Window(width, height, title.str(), mEglNativeDisplay, mNativeVisualId);
        gWinNameToNativeWindowMap[win] = window;
    }

    handle = new EglDrawable(width, height, mEglDisplay, mEglConfig, window, attribList);

    return handle;
}

GLWS& GLWS::instance()
{
    static GlwsEglX11 g;
    return g;
}
} // namespace retracer
