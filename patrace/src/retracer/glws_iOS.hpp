#if !defined(GLWS_EGL_IOS_HPP)
#define GLWS_EGL_IOS_HPP

#include "retracer/glws.hpp"

namespace retracer {
class GlwsIos : public GLWS
{
public:
    GlwsIos();
    ~GlwsIos();
    virtual void Init(Profile profile = PROFILE_ES2);
    virtual void Cleanup(void);
    virtual Drawable* CreateDrawable(int width, int height, int win, EGLint const* attribList);
    virtual Drawable* CreatePbufferDrawable(EGLint const* attrib_list);
    virtual Context* CreateContext(Context *shareContext, Profile profile);
    virtual bool MakeCurrent(Drawable *drawable, Context *context);
    virtual EGLImageKHR createImageKHR(Context* context, EGLenum target, uintptr_t buffer, const EGLint* attrib_list);
    virtual EGLBoolean destroyImageKHR(EGLImageKHR image);
    virtual bool setAttribute(Drawable* drawable, int attribute, int value);
};

}

#endif // !defined(GLWS_EGL_IOS_HPP)
