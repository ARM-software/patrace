#include "retracer/glws_iOS.hpp"
#include "retracer/retracer.hpp"
#include "retracer/retrace_api.hpp"
#include "retracer/trace_executor.hpp"
#include "retracer/forceoffscreen/offscrmgr.h"
#include "dispatch/eglproc_auto.hpp"
#include "pa_exception.h"

#include <assert.h>

using namespace common;

namespace retracer
{
    class CAEAGLDrawable : public Drawable
    {
    public:
        CAEAGLDrawable(int w, int h):
        Drawable(w, h) { }

        void swapBuffers()
        {
            // no need to swap buffer here.
        }

        void swapBuffersWithDamage(int *rects, int n_rects)
        {
            // no need to swap buffer here.
        }

        bool mIsPbuffer;
        int mTid;
    };

    GlwsIos::GlwsIos()
     : GLWS()
    {

    }

    GlwsIos::~GlwsIos()
    {

    }


    void GlwsIos::Init(Profile profile)
    {
        gApiInfo.RegisterEntries(gles_callbacks);
        gApiInfo.RegisterEntries(egl_callbacks);
    }

    void GlwsIos::Cleanup(void)
    {

    }

    Drawable* GlwsIos::CreateDrawable(int width, int height, int win, EGLint const* attribList)
    {
        DBG_LOG("Create CAEAGLDrawable(w=%d, h=%d)\n", width, height);
        return new CAEAGLDrawable(width, height, attribList);
    }

    Context* GlwsIos::CreateContext(Context *shareContext, Profile profile)
    {
        return new Context(profile, shareContext);
    }

    bool GlwsIos::MakeCurrent(Drawable *drawable, Context *context)
    {
        return true;
    }

    Drawable* GlwsIos::CreatePbufferDrawable(EGLint const* attrib_list)
    {
        DBG_LOG("CreatePbufferDrawable not supported\n");
        TraceExecutor::writeError("CreatePbufferDrawable not supported");
        throw PA_EXCEPTION("CreatePbufferDrawable not supported");
        return NULL;
    }

    EGLImageKHR GlwsIos::createImageKHR(Context* context, EGLenum target, uintptr_t buffer, const EGLint* attrib_list)
    {
        DBG_LOG("GlwsIos::createImageKHR not supported\n");
        TraceExecutor::writeError("GlwsIos::createImageKHR not supported");
        throw PA_EXCEPTION("GlwsIos::createImageKHR not supported");
    }

    EGLBoolean GlwsIos::destroyImageKHR(EGLImageKHR image)
    {
        DBG_LOG("GlwsIos::setAttribute not supported\n");
        TraceExecutor::writeError("GlwsIos::destroyImageKHR not supported");
        throw PA_EXCEPTION("GlwsIos::destroyImageKHR not supported");
    }

    bool GlwsIos::setAttribute(Drawable* drawable, int attribute, int value)
    {
        DBG_LOG("GlwsIos::setAttribute not supported\n");
        TraceExecutor::writeError("GlwsIos::setAttribute not supported");
        throw PA_EXCEPTION("GlwsIos::setAttribute not supported");
    }

    GLWS& GLWS::instance()
    {
        static GlwsIos g;
        return g;
    }
}
