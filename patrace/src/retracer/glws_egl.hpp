#if !defined(GLWS_EGL_HPP)
#define GLWS_EGL_HPP

#include "retracer/glws.hpp"
#include "common/os_thread.hpp"

namespace retracer {

typedef std::unordered_map<int, NativeWindow*> WinNameToNativeWindowMap_t;

class GlwsEgl : public GLWS
{
public:
    GlwsEgl();
    ~GlwsEgl();
    virtual void Init(Profile profile = PROFILE_ES2);
    virtual void Cleanup(void);
    virtual Drawable* CreateDrawable(int width, int height, int win, EGLint const* attribList);
    virtual Drawable* CreatePbufferDrawable(EGLint const* attrib_list);
    virtual Context* CreateContext(Context *shareContext, Profile profile);
    virtual bool MakeCurrent(Drawable *drawable, Context *context);
    virtual EGLImageKHR createImageKHR(Context* context, EGLenum target, uintptr_t buffer, const EGLint* attrib_list);
    virtual EGLBoolean destroyImageKHR(EGLImageKHR image);
    virtual bool setAttribute(Drawable* drawable, int attribute, int value);
    virtual void processStepEvent();
    virtual void releaseNativeDisplay(EGLNativeDisplayType display);

    virtual void postInit();
    virtual EGLNativeDisplayType getNativeDisplay();
    void setNativeWindow(EGLNativeWindowType window);

protected:
    EGLNativeDisplayType mEglNativeDisplay;
    EGLNativeWindowType mEglNativeWindow;
    EGLConfig mEglConfig;
    EGLDisplay mEglDisplay;
    EGLint mNativeVisualId;
    WinNameToNativeWindowMap_t gWinNameToNativeWindowMap;
    os::Mutex NativeWindowMutex;
};

class EglContext : public Context
{
public:
    EglContext(Profile profile, EGLDisplay eglDisplay, EGLContext ctx, Context* shareContext);
    ~EglContext();

    EGLDisplay mEglDisplay;
    EGLContext mContext;
};

class EglDrawable : public Drawable
{
public:
    EglDrawable(int w, int h, EGLDisplay eglDisplay, EGLConfig eglConfig, NativeWindow* nativeWindow, EGLint const* attribList);
    virtual ~EglDrawable();

    void resize(int w, int h);
    void show();
    void swapBuffers();
    void swapBuffersWithDamage(int *rects, int n_rects);

    EGLSurface getSurface() const { return mSurface; }

    virtual void setDamage(int* array, int length);
    virtual void querySurface(int attribute, int* value);

private:
    EGLSurface _createWindowSurface();

    EGLDisplay mEglDisplay;
    EGLConfig mEglConfig;
    NativeWindow* mNativeWindow;
    EGLSurface mSurface;
    std::vector<int> mAttribList;
};

class EglPbufferDrawable : public PbufferDrawable
{
public:
    EglPbufferDrawable(EGLDisplay eglDisplay, EGLConfig eglConfig, EGLint const* attribList);
    virtual ~EglPbufferDrawable();

    virtual void swapBuffers(void) {}
    virtual void swapBuffersWithDamage(int *rects, int n_rects) {}

    EGLSurface getSurface() const { return mSurface; }

    EGLDisplay mEglDisplay;
    EGLSurface mSurface;
};

int PAFW_Choose_EGL_Config(const EGLint* preferred_config, EGLDisplay display, EGLConfig configs[], EGLint numConfigs);


}

#endif // !defined(GLWS_EGL_HPP)
