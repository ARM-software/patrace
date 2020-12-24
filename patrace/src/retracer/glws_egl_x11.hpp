#if !defined(GLWS_EGL_X11_HPP)
#define GLWS_EGL_X11_HPP

#include "retracer/glws_egl.hpp"

namespace retracer {
class GlwsEglX11 : public GlwsEgl
{
public:
    GlwsEglX11();
    ~GlwsEglX11();

    virtual Drawable* CreateDrawable(int width, int height, int win, EGLint const* attribList) override;
    virtual void processStepEvent() override;
    virtual bool steppable() override { return true; }
    virtual EGLNativeDisplayType getNativeDisplay() override;
    virtual void releaseNativeDisplay(EGLNativeDisplayType display) override;
};

}

#endif // !defined(GLWS_EGL_X11_HPP)
