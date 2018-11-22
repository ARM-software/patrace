#include "iOS_API.h"

#include <retracer/retracer.hpp>
#include <retracer/trace_executor.hpp>
#include <retracer/glws.hpp>

using namespace retracer;
using namespace os;

// Used to store state at end of each frame, so it can be restored.
// BoundFBO == -1 means no old state to restore.
static GLint boundFBO = -1;
static GLint boundViewport[4];
static GLint boundScissor[4];

int iOSRetracer_retraceFile(const char* fileName)
{
    GLWS::instance().Init();

    // Make sure previous run is closed
    gRetracer.CloseTraceFile();

    int retValue = gRetracer.OpenTraceFile(fileName);
    gRetracer.mCurFrameNo = 0;

    // Assumes iPhone5s
    gRetracer.mOptions.mWindowWidth = 1136;
    gRetracer.mOptions.mWindowHeight = 640;

    gRetracer.mOptions.mForceOffscreen = false;

    boundFBO = -1;

    return (retValue == true) ? 0 /*OK*/ : 1 /*error*/;
}

iOSRetracer_TraceState iOSRetracer_retraceUntilSwapBuffers()
{
    // Restore old state
    if (boundFBO != -1)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, boundFBO);
        glViewport(boundViewport[0], boundViewport[1], boundViewport[2], boundViewport[3]);
        glScissor(boundScissor[0], boundScissor[1], boundScissor[2], boundScissor[3]);
    }

    bool shouldContinue = gRetracer.RetraceUntilSwapBuffers();

    // Save state
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundFBO);
    glGetIntegerv(GL_VIEWPORT, boundViewport);
    glGetIntegerv(GL_SCISSOR_BOX, boundScissor);

    if (shouldContinue)
    {
        // Trace not done yet
        return iOSRetracer_TRACE_NOT_FINISHED;
    }
    else
    {
        // Stopped for some reason. Return why.
        boundFBO = -1;

        if(gRetracer.mOutOfMemory)
            return iOSRetracer_TRACE_FAILED_OUT_OF_MEMORY;

        if(gRetracer.mFailedToLinkShaderProgram)
            return iOSRetracer_TRACE_FAILED_FAILED_TO_LINK_SHADER_PROGRAM;

        // Assume it went OK
        return iOSRetracer_TRACE_FINISHED_OK;
    }
}

int iOSRetracer_initWithJSON(const char* json, const char* storagePath, const char* resultFile)
{
    TraceExecutor::initFromJson(json, storagePath, resultFile);
    boundFBO = -1;
    return 0;
}

retracer::RetraceOptions* iOSRetracer_getRetracerOptions()
{
    return &gRetracer.mOptions;
}

int iOSRetracer_setAndCheckSelectedConfig(SelectedConfig config)
{
   // Register what was actually selected
    GLWS::instance().setSelectedEglConfig(config.eglConfig);

    int err = 0;
    
    // Check selected config is valid
    if (!gRetracer.mOptions.mOnscreenConfig.isSane(config.eglConfig))
    {
        err = 1;
    }

    if(!gRetracer.mOptions.mForceOffscreen)
    {
        // Check screen-surface is big enough, else fail
        if(config.surfaceHeight < gRetracer.mOptions.mWindowHeight || config.surfaceWidth < gRetracer.mOptions.mWindowWidth)
        {
            // Surface too small
            TraceExecutor::writeError(TraceExecutorErrorCode::TRACE_ERROR_OUT_OF_MEMORY, "Invalid configuration selected: surface too small. ");
            err = 1;
        }
    }

    return err;
}

void iOSRetracer_setDebug(bool on)
{
    gRetracer.mOptions.mDebug = on;
}

void iOSRetracer_closeTraceFile()
{
    gRetracer.CloseTraceFile();
}
