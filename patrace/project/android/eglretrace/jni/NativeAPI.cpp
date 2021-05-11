#include "com_arm_pa_paretrace_NativeAPI.h"

#include "retracer/retracer.hpp"
#include "retracer/glws_egl_android.hpp"
#include "retracer/trace_executor.hpp"
#include "retracer/retrace_api.hpp"

#include "common/os_time.hpp"
#include "common/trace_callset.hpp"
#include "jsoncpp/include/json/reader.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/prctl.h>
#include <linux/prctl.h>

using namespace std;
using namespace retracer;
using namespace os;

JavaVM* gJavaVM;
jint    gJavaVersion; // used in retracer.cpp

JNIEXPORT jboolean JNICALL Java_com_arm_pa_paretrace_NativeAPI_initFromJson(JNIEnv *env, jclass, jstring jstrJsonData, jstring jstrTraceDir, jstring jstrResultFile)
{
    std::string resultFile;

    if (jstrResultFile != NULL)
    {
        const char* cresultFile = env->GetStringUTFChars(jstrResultFile, 0);
        resultFile = cresultFile;
        env->ReleaseStringUTFChars(jstrResultFile, cresultFile);
    }

    const char* jsonData = env->GetStringUTFChars(jstrJsonData, 0);
    const char* traceDir = env->GetStringUTFChars(jstrTraceDir, 0);

    TraceExecutor::initFromJson(jsonData, traceDir, resultFile);
    common::gApiInfo.RegisterEntries(gles_callbacks);
    common::gApiInfo.RegisterEntries(egl_callbacks);
    gRetracer.OpenTraceFile(gRetracer.mOptions.mFileName.c_str());

    env->ReleaseStringUTFChars(jstrJsonData, jsonData);
    env->ReleaseStringUTFChars(jstrTraceDir, traceDir);

    return true;
}

JNIEXPORT void JNICALL Java_com_arm_pa_paretrace_NativeAPI_init(JNIEnv * env, jclass, jboolean registerEntries)
{
    prctl(PR_SET_DUMPABLE, 1); // enable debugging

    gJavaVersion = env->GetVersion();
    if (env->GetJavaVM(&gJavaVM) != 0)
    {
        DBG_LOG("Failed to get JavaVM\n");
    }

    GlwsEglAndroid* g = dynamic_cast<GlwsEglAndroid*>(&GLWS::instance());
    g->Init();
    g->setupJAVAEnv(env);
}

JNIEXPORT jboolean JNICALL Java_com_arm_pa_paretrace_NativeAPI_step(JNIEnv *env, jclass)
{
    gRetracer.Retrace();
    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_arm_pa_paretrace_NativeAPI_stop(JNIEnv *env, jclass, jstring js)
{
    DBG_LOG("Java_com_arm_pa_paretrace_NativeAPI_stop\n");
    gRetracer.mFinish = true;
    std::string cjs;
    if (js != nullptr)
    {
        const char* cstr = env->GetStringUTFChars(js, 0);
        cjs = cstr;
        env->ReleaseStringUTFChars(js, cstr);
    }
    Json::Value results;
    Json::Reader reader;
    if (!reader.parse(cjs, results))
    {
        gRetracer.reportAndAbort("JSON parse error: %s\n", reader.getFormattedErrorMessages().c_str());
    }
    gRetracer.saveResult(results);
}

JNIEXPORT jint JNICALL Java_com_arm_pa_paretrace_NativeAPI_opt_1getAPIVersion(JNIEnv *env, jclass)
{
    return gRetracer.mOptions.mApiVersion;
}

JNIEXPORT jboolean JNICALL Java_com_arm_pa_paretrace_NativeAPI_opt_1getIsPortraitMode(JNIEnv *, jclass)
{
    return gRetracer.mOptions.mWindowHeight > gRetracer.mOptions.mWindowWidth;
}

JNIEXPORT void JNICALL Java_com_arm_pa_paretrace_NativeAPI_setSurface(JNIEnv* env, jclass obj, jobject surface, jint viewSize)
{
    DBG_LOG("nativeSetSurface\n");
    static ANativeWindow* sWindow = 0;

    if (surface)
    {
        sWindow = ANativeWindow_fromSurface(env, surface);
        DBG_LOG("Got window %p\n", sWindow);
        GlwsEglAndroid* g = dynamic_cast<GlwsEglAndroid*>(&GLWS::instance());
        g->setNativeWindow(sWindow, viewSize);
    }
    else
    {
        DBG_LOG("Releasing window\n");
        ANativeWindow_release(sWindow);
    }
}
