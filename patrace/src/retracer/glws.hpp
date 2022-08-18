#ifndef _RETRACER_GLWS_HPP_
#define _RETRACER_GLWS_HPP_

#include "retracer/eglconfiginfo.hpp"
#include "retracer/state.hpp"
#include "json/value.h"
#include "EGL/egl.h"
#include <ostream>

namespace retracer {

class NativeWindow
{
public:
    NativeWindow(int width, int height, const std::string& title);
    virtual ~NativeWindow() {}
    virtual void show();
    virtual void close();
    virtual bool resize(int w, int h);
    virtual EGLNativeWindowType getHandle() const { return mHandle; }

    void setWidth(int width) { mWidth = width; }
    void setHeight(int height) { mHeight = height; }

    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }

protected:
    EGLNativeWindowType mHandle;
    bool mVisible;

private:
    NativeWindow(const NativeWindow& other);
    NativeWindow& operator= (const NativeWindow&);
    int mWidth;
    int mHeight;
};

class GLWS
{
public:
    static GLWS& instance();
    virtual void Init(Profile profile = PROFILE_ES2) = 0;
    virtual void Cleanup(void) = 0;
    virtual Drawable* CreateDrawable(int width, int height, int win, EGLint const* attribList) = 0;
    virtual void ReleaseDrawable(NativeWindow *window) = 0;
    virtual Drawable* CreatePbufferDrawable(EGLint const* attrib_list) = 0;
    virtual Context* CreateContext(Context *shareContext, Profile profile) = 0;
    virtual bool MakeCurrent(Drawable *drawable, Context *context) = 0;
    virtual EGLImageKHR createImageKHR(Context* context, EGLenum target, uintptr_t buffer, const EGLint* attrib_list) = 0;
    virtual EGLBoolean destroyImageKHR(EGLImageKHR image) = 0;
    virtual bool setAttribute(Drawable* drawable, int attribute, int value) = 0;
    virtual bool querySupportedCompressionRates(const EGLAttrib *attrib_list, EGLint *rates, EGLint rate_size, EGLint *num_rates) = 0;

    virtual void processStepEvent() {}
    virtual bool steppable() { return false; }

    void setSelectedEglConfig(const EglConfigInfo& config) { mEglConfigInfo = config; }
    EglConfigInfo getSelectedEglConfig() const { return mEglConfigInfo; }
    Json::Value getEglInfoJson() const { return mEglJson; }

protected:
    GLWS() : mEglConfigInfo() {}
    virtual ~GLWS() {}
    EglConfigInfo mEglConfigInfo;
    Json::Value mEglJson;

private:
    GLWS(const GLWS& other);
    GLWS& operator= (const GLWS&);
};

}

#endif
