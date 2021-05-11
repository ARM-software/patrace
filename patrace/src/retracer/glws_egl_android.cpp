#include "retracer/glws_egl_android.hpp"
#include "retracer/retracer.hpp"

#include "dispatch/eglproc_auto.hpp"

#include <android/native_window.h>

#include <sstream>

using namespace common;

namespace retracer
{

class AndroidWindow : public NativeWindow
{
public:
    AndroidWindow(int width, int height, EGLNativeWindowType window, EGLint nativeVisualId)
        : NativeWindow(width, height, "0")
    {
        mHandle = window;
        mNativeVisualId = nativeVisualId;
    }

    ~AndroidWindow()
    {
    }

    bool resize(int w, int h)
    {
        if (NativeWindow::resize(w, h))
        {
            eglWaitClient();
            eglWaitNative(EGL_CORE_NATIVE_ENGINE);
            return true;
        }

        return false;
    }

private:
    EGLint mNativeVisualId;
};

GlwsEglAndroid::GlwsEglAndroid()
    : GlwsEgl()
{
}

GlwsEglAndroid::~GlwsEglAndroid()
{
}

EGLImageKHR GlwsEglAndroid::createImageKHR(Context* context, EGLenum target, uintptr_t buffer, const EGLint* attrib_list)
{
    EGLContext eglContext = EGL_NO_CONTEXT;
    if (context)
    {
        eglContext = static_cast<EglContext*>(context)->mContext;
    }

    EGLenum _target = target;

    uintptr_t uintBuffer = static_cast<uintptr_t>(buffer);
    EGLClientBuffer eglBuffer = reinterpret_cast<EGLClientBuffer>(uintBuffer);
    return _eglCreateImageKHR(mEglDisplay, eglContext, _target, eglBuffer, attrib_list);
}

void GlwsEglAndroid::postInit()
{
    gWinNameToNativeWindowMap.clear();
}

void GlwsEglAndroid::setupJAVAEnv(JNIEnv *env)
{
    env->GetJavaVM(&jvm);
    jclass cls = env->FindClass("com/arm/pa/paretrace/NativeAPI");
    gNativeCls = reinterpret_cast<jclass>(env->NewGlobalRef(cls));
    if (gNativeCls == NULL)
    {
        DBG_LOG("ERROR: Can't get NativeAPI class!\n");
        exit(1);
    }

    gRequestWindowID = env->GetStaticMethodID(gNativeCls, "requestNewSurfaceAndWait", "(III)V");
    if (gRequestWindowID == NULL)
    {
        DBG_LOG("ERROR: Can't get requestNewSurfaceAndWait api!\n");
        exit(1);
    }

    gResizeWindowID = env->GetStaticMethodID(gNativeCls, "resizeNewSurfaceAndWait", "(IIII)V");
    if (gResizeWindowID == NULL)
    {
        DBG_LOG("ERROR: Can't get resizeNewSurfaceAndWait api!\n");
        exit(1);
    }

    gDestroyWindowID = env->GetStaticMethodID(gNativeCls, "destroySurfaceAndWait", "(I)V");
    if (gDestroyWindowID == NULL)
    {
        DBG_LOG("ERROR: Can't get destroySurfaceAndWait api!\n");
        exit(1);
    }
}

void GlwsEglAndroid::requestNativeWindow(int width, int height, int format)
{
    JNIEnv* jEnv = static_cast<JNIEnv*>(retracer::gRetracer.mState.mThreadArr[retracer::gRetracer.getCurTid()].mpJavaEnv);

    if (jEnv == NULL)
    {
        int ret = jvm->AttachCurrentThread(&jEnv, NULL);
        if (ret != 0)
        {
            DBG_LOG("ERROR: Can't get JNI interface! For thread %d.\n",retracer::gRetracer.getCurTid());
            exit(1);
        }
        retracer::gRetracer.mState.mThreadArr[retracer::gRetracer.getCurTid()].mpJavaEnv = jEnv;
    }

    jEnv->CallStaticVoidMethod(gNativeCls, gRequestWindowID, width, height, format);
}

void GlwsEglAndroid::resizeNativeWindow(int width, int height, int format, int viewId)
{
    JNIEnv* jEnv = static_cast<JNIEnv*>(retracer::gRetracer.mState.mThreadArr[retracer::gRetracer.getCurTid()].mpJavaEnv);

    if (jEnv == NULL)
    {
        int ret = jvm->AttachCurrentThread(&jEnv, NULL);
        if (ret != 0)
        {
            DBG_LOG("ERROR: Can't get JNI interface! For thread %d.\n",retracer::gRetracer.getCurTid());
            exit(1);
        }
        retracer::gRetracer.mState.mThreadArr[retracer::gRetracer.getCurTid()].mpJavaEnv = jEnv;
    }

    jEnv->CallStaticVoidMethod(gNativeCls, gResizeWindowID, width, height, format, viewId);
}

void GlwsEglAndroid::destroyNativeWindow(int viewId)
{
    JNIEnv* jEnv = static_cast<JNIEnv*>(retracer::gRetracer.mState.mThreadArr[retracer::gRetracer.getCurTid()].mpJavaEnv);

    if (jEnv == NULL)
    {
        int ret = jvm->AttachCurrentThread(&jEnv, NULL);
        if (ret != 0)
        {
            DBG_LOG("ERROR: Can't get JNI interface! For thread %d.\n",retracer::gRetracer.getCurTid());
            exit(1);
        }
        retracer::gRetracer.mState.mThreadArr[retracer::gRetracer.getCurTid()].mpJavaEnv = jEnv;
    }

    jEnv->CallStaticVoidMethod(gNativeCls, gDestroyWindowID, viewId);
}

void GlwsEglAndroid::syncNativeWindow()
{
    eglWaitClient();
    eglWaitNative(EGL_CORE_NATIVE_ENGINE);
}

Drawable* GlwsEglAndroid::CreateDrawable(int width, int height, int win, EGLint const* attribList)
{
    Drawable* handler = NULL;
    WinNameToNativeWindowMap_t::iterator it = gWinNameToNativeWindowMap.find(win);
    std::unordered_map<int, int>::iterator it2 = winNameToViewIdMap.find(win);

    NativeWindow* window = NULL;
    if (it != gWinNameToNativeWindowMap.end())
    {
        window = it->second;
        if (window->getWidth() != width ||
            window->getHeight() != height)
        {
            window->resize(width, height);
            int viewId = 0;
            if (it2 != winNameToViewIdMap.end()) {
                viewId = it2->second;
            }
            resizeNativeWindow(width, height, mNativeVisualId, viewId);
            ANativeWindow_setBuffersGeometry(mEglNativeWindow, width, height, mNativeVisualId);
        }
    }
    else if (!gRetracer.mOptions.mPbufferRendering)
    {
        syncNativeWindow();
        mEglNativeWindow = NULL;
        requestNativeWindow(width, height, mNativeVisualId);
        if (mEglNativeWindow == NULL)
        {
            DBG_LOG("ERROR: Can't create Native window!\n");
            return NULL;
        }
        // Sets up correct pixel format on Android's window
        ANativeWindow_setBuffersGeometry(mEglNativeWindow, width, height, mNativeVisualId);
        window = new AndroidWindow(width, height, mEglNativeWindow, mNativeVisualId);
        gWinNameToNativeWindowMap[win] = window;
        winNameToViewIdMap[win] = mViewSize - 1;
    }

    handler = new EglDrawable(width, height, mEglDisplay, mEglConfig, window, attribList);

    return handler;
}

void GlwsEglAndroid::ReleaseDrawable(NativeWindow *window)
{
    for (WinNameToNativeWindowMap_t::iterator it = gWinNameToNativeWindowMap.begin(); it != gWinNameToNativeWindowMap.end(); ++it)
    {
        if (it->second == window)
        {
            destroyNativeWindow( winNameToViewIdMap[it->first] );
            gWinNameToNativeWindowMap.erase(it);
        }
    }
    if (window) delete window;
}

void GlwsEglAndroid::setNativeWindow(EGLNativeWindowType window, int viewSize)
{
    mEglNativeWindow = window;
    mViewSize = viewSize;
}

GLWS& GLWS::instance()
{
    static GlwsEglAndroid g;
    return g;
}
} // namespace retracer
