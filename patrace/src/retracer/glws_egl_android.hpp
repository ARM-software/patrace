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
    virtual void postInit();
    void setupJAVAEnv(JNIEnv *env);
    virtual Drawable* CreateDrawable(int width, int height, int win);
    void setNativeWindow(EGLNativeWindowType window, int textureViewSize);

private:
    JavaVM *jvm;
    jclass gNativeCls;
    jmethodID gRequestWindowID;
    jmethodID gResizeWindowID;
    int mTextureViewSize;
    std::unordered_map<int, int> winNameToTextureViewIdMap;

    void requestNativeWindow(int width, int height, int format);
    void resizeNativeWindow(int width, int height, int format, int textureViewId);
};

}

#endif // !defined(GLWS_EGL_ANDROID_HPP)
