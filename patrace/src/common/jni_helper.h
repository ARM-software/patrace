#if !defined(JNI_HELPER_H)
#define JNI_HELPER_H

#include "common/pa_exception.h"
#include "os.hpp"
#include <string>
#include <sstream>
#include <jni.h>

// A long-lived native object corresponding to a Java object
class Peer
{
public:
    Peer()
        : m_globalRef(0)
    {}

    Peer(JNIEnv* env, jobject localRef, const std::string& name=std::string())
        : m_globalRef(env->NewGlobalRef(localRef))
        , m_name(name)
    {}

    ~Peer()
    {
        if (m_globalRef)
        {
            DBG_LOG("Peer '%s': m_globalRef should be 0.\n", m_name.c_str());
            exit(1);
        }
    }

    void create(JNIEnv* env, jobject localRef, const std::string& name=std::string())
    {
        m_globalRef = env->NewGlobalRef(localRef);
        m_name = name;
    }

    void destroy(JNIEnv* env)
    {
        env->DeleteGlobalRef(m_globalRef);
        m_globalRef = 0;
    }

    jobject operator * ()
    {
        return m_globalRef;
    }

    operator jobject()
    {
        return m_globalRef;
    }

private:
    jobject m_globalRef;
    // Optional name, that is used for debugging
    std::string m_name;
};

class JNIClass
{
public:
    JNIClass(JNIEnv* env, const std::string& name)
        : m_class(env->FindClass(name.c_str()))
        , m_name(name) 
    {
        if (!m_class)
        {
            throw PA_EXCEPTION("Unable to find class " + m_name);
        }
    }

    jclass operator * ()
    {
        return m_class;
    }

    operator jobject()
    {
        return m_class;
    }

    //getIntField(
    //jfieldID fid_chosen_egl_red = pEnv->GetFieldID(*cls_chosen_egl_info, "colorBitsRed", "I");
    //eglConfigInfo.red = pEnv->GetIntField(obj_chosen_egl, fid_chosen_egl_red);

private:
    jclass m_class;
    std::string m_name;
};
//class PeerClass : public Peer
//{
//    PeerClass(JNIEnv* env, const std::string& className))
//        : m_globalRef(pEnv->FindClass(className.c_str()))
//        , m_name(className)
//    {}
//};

#endif  /* JNI_HELPER_H */
