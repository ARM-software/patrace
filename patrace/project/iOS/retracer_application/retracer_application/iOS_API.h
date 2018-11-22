#pragma once
#include "retracer/retrace_options.hpp" // retracer::RetraceOptions
#include "retracer/eglconfiginfo.hpp"   // EglConfigInfo

// For manual retraces. Returns nonzero on errors.
//int iOSRetracer_init(int w, int h);
int iOSRetracer_retraceFile(const char* fileName);

// Used by gateway client. Returns nonzero on error.
int iOSRetracer_initWithJSON(const char* json, const char* storagePath, const char* resultFile);

// Used to set up GL-state (e.g. context version, color/depth-bits etc.)
retracer::RetraceOptions* iOSRetracer_getRetracerOptions();

struct SelectedConfig {
    EglConfigInfo eglConfig;
    int surfaceWidth;
    int surfaceHeight;
};

// After context-creation, call this to register the state used.
// Returns non-zero if the state if not OK to run the trace.
int iOSRetracer_setAndCheckSelectedConfig(SelectedConfig config);

// Sets mOptions.mDebug
void iOSRetracer_setDebug(bool on);

typedef enum {
    iOSRetracer_TRACE_NOT_FINISHED,
    iOSRetracer_TRACE_FINISHED_OK,
    iOSRetracer_TRACE_FAILED_OUT_OF_MEMORY,
    iOSRetracer_TRACE_FAILED_FAILED_TO_LINK_SHADER_PROGRAM,
    iOSRetracer_TRACE_FAILED
} iOSRetracer_TraceState;

// Draws the next frame. Check returned value to know
// if retrace should continue or why it shouldn't.
iOSRetracer_TraceState iOSRetracer_retraceUntilSwapBuffers();

// Call when aborting trace prematurely (otherwise called automatically at end of retrace)
void iOSRetracer_closeTraceFile();