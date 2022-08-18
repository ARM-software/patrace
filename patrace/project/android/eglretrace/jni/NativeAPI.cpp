#include "com_arm_pa_paretrace_NativeAPI.h"

#include "retracer/retracer.hpp"
#include "retracer/glws_egl_android.hpp"
#include "retracer/trace_executor.hpp"
#include "retracer/retrace_api.hpp"

#include "common/os_time.hpp"
#include "common/trace_callset.hpp"
#include "json/reader.h"

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

int ff_main(int argc, char** argv); // declaration of ff_main()

JNIEXPORT jboolean JNICALL Java_com_arm_pa_paretrace_NativeAPI_launchFastforward(JNIEnv * env, jclass, jobjectArray jstrArrayArgs)
{
    prctl(PR_SET_DUMPABLE, 1); // enable debugging

    gJavaVersion = env->GetVersion();
    if (env->GetJavaVM(&gJavaVM) != 0)
    {
        DBG_LOG("Failed to get JavaVM\n");
    }

    GlwsEglAndroid* g = dynamic_cast<GlwsEglAndroid*>(&GLWS::instance());

    g->setupJAVAEnv(env);

    int argc = env->GetArrayLength(jstrArrayArgs) + 1;
    const char** strArray = new const char*[argc];
    strArray[0] = "fastforward";

    for (int i = 1; i < argc; i++)
    {
        jstring str = (jstring) (env->GetObjectArrayElement(jstrArrayArgs, i - 1));
        strArray[i] = env->GetStringUTFChars(str, 0);
    }

    env->DeleteLocalRef(jstrArrayArgs);

    for (int i = 0; i < argc; i++)
    {
        DBG_LOG("fastforward args: %s", strArray[i]);
    }

    char** argv = const_cast<char**>(strArray);

    ff_main(argc, argv);
    return JNI_TRUE;
}

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
    common::gApiInfo.RegisterEntries(gles_callbacks, gRetracer.mOptions.mRunAll);
    common::gApiInfo.RegisterEntries(egl_callbacks, gRetracer.mOptions.mRunAll);
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
        gRetracer.reportAndAbort("JSON parse error: %s", reader.getFormattedErrorMessages().c_str());
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

    if (viewSize > 0)
    {
        sWindow = ANativeWindow_fromSurface(env, surface);
        DBG_LOG("Got window %p\n", sWindow);
        GlwsEglAndroid* g = dynamic_cast<GlwsEglAndroid*>(&GLWS::instance());
        g->setNativeWindow(sWindow, viewSize);
    }
    else
    {
        sWindow = ANativeWindow_fromSurface(env, surface);
        DBG_LOG("Releasing window %p\n", sWindow);
        ANativeWindow_release(sWindow);
    }
}

