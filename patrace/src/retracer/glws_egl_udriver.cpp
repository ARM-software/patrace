#include "retracer/glws_egl_udriver.hpp"
#include "retracer/retracer.hpp"

#include "dispatch/eglproc_auto.hpp"
#include "WindowSystem.h"


namespace retracer
{

class UdriverWindow : public NativeWindow
{
public:
    UdriverWindow(int width, int height, const std::string& title)
        : NativeWindow(width, height, title)
    {
        pWindow = WindowSystem::CreateWin(
                        NULL,
                        width,
                        height,
                        title.c_str(),
                        GFormats::COLOR_FORMAT_RGBA8,
                        GFormats::DEPTH_STENCIL_FORMAT_NONE);
    }

    EGLNativeWindowType getHandle() const { return (EGLNativeWindowType)pWindow; }

    bool resize(int w, int h)
    {
        //Resize is not supported by udriver window
        return false;
    }

private:
    WindowSystem::Window *pWindow = NULL;
};


Drawable* GlwsEglUdriver::CreateDrawable(int width, int height, int /*win*/, EGLint const* attribList)
{
    Drawable* handler = NULL;
    NativeWindowMutex.lock();
    gWinNameToNativeWindowMap[0] = new UdriverWindow(width, height, "0");
    handler = new EglDrawable(width, height, mEglDisplay, mEglConfig, gWinNameToNativeWindowMap[0], attribList);
    NativeWindowMutex.unlock();
    return handler;
}

GlwsEglUdriver::GlwsEglUdriver()
    : GlwsEgl()
{
}

GlwsEglUdriver::~GlwsEglUdriver()
{
}

void usage()
{
    printf("Cmd usage:\n");
    printf("    s: render one draw call.\n");
    printf("    n: render one frame.\n");
    printf("    q: exit.\n");
    printf("    h: show this usage.\n");
}

void GlwsEglUdriver::processStepEvent()
{
    char cmd;
    usage();
    while (1)
    {
        cmd = getchar();
        if (cmd == 0xa)
        {
            continue;
        }
        else if (cmd == 'n' || cmd == 'N')
        {
            if (!gRetracer.RetraceForward(1, 0, false))
                return;
        }
        else if (cmd == 's' || cmd == 'S')
        {
            if (!gRetracer.RetraceForward(0, 1, false))
                return;
        }
        else if (cmd == 'q' || cmd == 'Q')
        {
            gRetracer.CloseTraceFile();
            return;
        }
        else if (cmd == 'h' || cmd == 'H')
        {
            usage();
        }
    }
}

GLWS& GLWS::instance()
{
    static GlwsEglUdriver g;
    return g;
}
} // namespace retracer
