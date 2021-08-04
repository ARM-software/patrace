#if !defined(GLWS_EGL_ANDROID_HPP)
#define GLWS_EGL_ANDROID_HPP

#include <jni.h>
#include <unordered_map>
#include "retracer/glws_egl.hpp"

namespace retracer {
class GlwsEglAndroid : public GlwsEgl
{
public:
    GlwsEglAndroid();
    ~GlwsEglAndroid();
    virtual EGLImageKHR createImageKHR(Context* context, EGLenum target, uintptr_t buffer, const EGLint* attrib_list) override;
    virtual void postInit() override;
    void setupJAVAEnv(JNIEnv *env);
    virtual Drawable* CreateDrawable(int width, int height, int win, EGLint const* attribList) override;
    virtual void ReleaseDrawable(NativeWindow *window) override;
    void setNativeWindow(EGLNativeWindowType window, int viewSize);

private:
    JavaVM *jvm;
    jclass gNativeCls;
    jmethodID gRequestWindowID;
    jmethodID gResizeWindowID;
    jmethodID gDestroyWindowID;
    int mViewSize;
    std::unordered_map<int, int> winNameToViewIdMap;

    void requestNativeWindow(int width, int height, int format, int win);
    void resizeNativeWindow(int width, int height, int format, int win);
    void destroyNativeWindow(int win);
    void syncNativeWindow();
};

}

#endif // !defined(GLWS_EGL_ANDROID_HPP)
