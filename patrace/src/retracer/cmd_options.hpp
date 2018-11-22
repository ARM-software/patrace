#ifndef _CMD_OPTIONS_HPP
#define _CMD_OPTIONS_HPP

#include "retracer/eglconfiginfo.hpp"
#include "common/trace_callset.hpp"
#include <string>

struct CmdOptions
{
    CmdOptions() : eglConfig(-1, -1, -1, -1, -1, -1, -1, -1) {}

    std::string fileName;
    bool printHeaderInfo = false;
    bool printHeaderJson = false;
    int tid = -1;
    int winW = -1;
    int winH = -1;
    int oresW = -1;
    int oresH = -1;
    bool stepMode = false;
    common::CallSet* snapshotCallSet = nullptr;
    bool snapshotFrameNames = false;
    int beginMeasureFrame = -1;
    int endMeasureFrame = -1;
    bool preload = false;
    int debug = 0;
    bool stateLogging = false;
    bool drawLogging = false;
    bool forceOffscreen = false;
    bool forceSingleWindow = false;
    bool multiThread = false;
    bool forceInSequence = false;
    // eglConfig is used to select fbo format in offscreen (FBO mode)
    EglConfigInfo eglConfig;
    bool strictEGLMode = false;
    bool strictColorMode = false;
    common::CallSet* skipCallSet = nullptr;
    bool singleFrameOffscreen = false;
    int skipWork = -1;
    bool dumpStatic = false;
    bool callStats = false;
    bool collectors = false;
    bool collectors_streamline = false;
    bool flushWork = false;
    bool pbufferRendering = false;
    std::string perfPath;
    std::string perfOut;
    int perfStart = -1;
    int perfStop = -1;
    int perfFreq = -1;
};

#endif
