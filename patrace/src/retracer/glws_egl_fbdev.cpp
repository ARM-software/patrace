#include "retracer/glws_egl_fbdev.hpp"
#include "retracer/retracer.hpp"

#include "dispatch/eglproc_auto.hpp"


namespace retracer
{

class FbdevWindow : public NativeWindow
{
public:
    FbdevWindow(int width, int height, const std::string& title)
        : NativeWindow(width, height, title)
        , mFbdevWindowStruct(width, height)
    {
        mHandle = reinterpret_cast<EGLNativeWindowType>(&mFbdevWindowStruct);
    }

    bool resize(int w, int h)
    {
        if (NativeWindow::resize(w, h))
        {
            mFbdevWindowStruct.setWidth(w);
            mFbdevWindowStruct.setHeight(h);
            return true;
        }

        return false;
    }

private:
    struct FbdevWindowStruct
    {
        FbdevWindowStruct(int width_, int height_)
            : width(static_cast<unsigned short>(width_))
            , height(static_cast<unsigned short>(height_))
        {}

        void setWidth(int width_)
        {
            width = static_cast<unsigned short>(width_);
        }

        void setHeight(int height_)
        {
            height = static_cast<unsigned short>(height_);
        }

        unsigned short width;
        unsigned short height;
    };

    FbdevWindowStruct mFbdevWindowStruct;
};


Drawable* GlwsEglFbdev::CreateDrawable(int width, int height, int /*win*/)
{
    // TODO: Delete
    // fbdev only supports one single native window. Therefor, map everything to 0.
    Drawable* handler = NULL;
    NativeWindowMutex.lock();
    gWinNameToNativeWindowMap[0] = new FbdevWindow(width, height, "0");
    handler = new EglDrawable(width, height, mEglDisplay, mEglConfig, gWinNameToNativeWindowMap[0]);
    NativeWindowMutex.unlock();
    return handler;
}

GlwsEglFbdev::GlwsEglFbdev()
    : GlwsEgl()
{
}

GlwsEglFbdev::~GlwsEglFbdev()
{
}

EGLImageKHR GlwsEglFbdev::createImageKHR(Context* context, EGLenum target, uintptr_t buffer, const EGLint* attrib_list)
{
    EGLContext eglContext = EGL_NO_CONTEXT;
    if (context)
    {
        eglContext = static_cast<EglContext*>(context)->mContext;
    }

    uintptr_t uintBuffer = static_cast<uintptr_t>(buffer);
    EGLClientBuffer eglBuffer = reinterpret_cast<EGLClientBuffer>(uintBuffer);
    return _eglCreateImageKHR(mEglDisplay, eglContext, target, eglBuffer, attrib_list);
}

GLWS& GLWS::instance()
{
    static GlwsEglFbdev g;
    return g;
}
} // namespace retracer
