#include "retracer/glws_egl_udriver.hpp"
#include "retracer/retracer.hpp"

#include "dispatch/eglproc_auto.hpp"
#include "WindowSystem.h"
#include <assert.h>

namespace retracer
{

class UdriverWindow : public NativeWindow
{
public:
    UdriverWindow(int width, int height, GFormats::ColorFormat color_format,
        GFormats::DepthStencilFormat ds_format, const std::string& title)
        : NativeWindow(width, height, title)
    {
        pWindow = WindowSystem::CreateWin(
                        NULL,
                        width,
                        height,
                        title.c_str(),
                        color_format,
                        ds_format);
    }

    EGLNativeWindowType getHandle() const { return (EGLNativeWindowType)pWindow; }

    bool resize(int w, int h)
    {
        // Resize is not supported by udriver window
        return false;
    }

private:
    WindowSystem::Window *pWindow = NULL;
};

void get_udriver_win_config(EglConfigInfo info, GFormats::ColorFormat &color_format, GFormats::DepthStencilFormat &ds_format)
{
    // For Color: Currently udriver only support RGBA8 and RGB8 two kinds of attachments,
    // choose one of them based on if alpha is used.
    // For DS: If no attachment is used, choose none. otherwise choose based on depth/stencil used bits.
    color_format = GFormats::COLOR_FORMAT_RGBA8;
    ds_format = GFormats::DEPTH_STENCIL_FORMAT_NONE;

    // Choose color format
    if (info.alpha == 0)
    {
        color_format = GFormats::COLOR_FORMAT_RGB8;
    }

    // Choose depth stencil format
    if (info.stencil > 0)
    {
        assert(info.depth == 24);
        ds_format = GFormats::DEPTH_STENCIL_FORMAT_D24_S8;
    }
    else if (info.depth > 0)
    {
        if (info.depth == 16)
        {
            ds_format = GFormats::DEPTH_STENCIL_FORMAT_D16;
        }
        else if (info.depth == 24)
        {
            ds_format = GFormats::DEPTH_STENCIL_FORMAT_D24;
        }
        else
        {
            assert(!"unsupported depth format");
        }
    }
}

Drawable* GlwsEglUdriver::CreateDrawable(int width, int height, int /*win*/, EGLint const* attribList)
{
    Drawable* handler = NULL;
    // Choose different format according to selected EglConfig
    GFormats::ColorFormat color_format;
    GFormats::DepthStencilFormat ds_format;
    get_udriver_win_config(this->getSelectedEglConfig(), color_format, ds_format);
    gWinNameToNativeWindowMap[0] = new UdriverWindow(width, height, color_format, ds_format, "0");
    handler = new EglDrawable(width, height, mEglDisplay, mEglConfig, gWinNameToNativeWindowMap[0], attribList);
    return handler;
}

static void usage()
{
    printf("Cmd usage:\n");
    printf("    D: render one draw call.\n");
    printf("    N: render one frame.\n");
    printf("    M: render 10 frames.\n");
    printf("    L: render 100 frames.\n");
    printf("    Q: exit.\n");
    printf("    H: show this usage.\n");
}

GlwsEglUdriver::GlwsEglUdriver()
    : GlwsEgl()
{
    if (gRetracer.mOptions.mStepMode) usage();
}

GlwsEglUdriver::~GlwsEglUdriver()
{
}

void GlwsEglUdriver::processStepEvent()
{
    const char cmd = getchar();

    if (cmd == 'N')
    {
        gRetracer.frameBudget++;
    }
    else if (cmd == 'M')
    {
        gRetracer.frameBudget += 10;
    }
    else if (cmd == 'L')
    {
        gRetracer.frameBudget += 100;
    }
    else if (cmd == 'D')
    {
        gRetracer.drawBudget++;
    }
    else if (cmd == 'Q')
    {
        gRetracer.mFinish = true;
    }
    else if (cmd == 'H')
    {
        usage();
    }
}

GLWS& GLWS::instance()
{
    static GlwsEglUdriver g;
    return g;
}
} // namespace retracer
