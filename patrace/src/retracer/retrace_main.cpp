#include <errno.h>

#include <retracer/retracer.hpp>
#include <retracer/glws.hpp>
#include <retracer/trace_executor.hpp>
#include <retracer/retrace_api.hpp>
#include <retracer/config.hpp>
#include <dispatch/eglproc_retrace.hpp>

#include "libcollector/interface.hpp"

#include "helper/states.h"

using namespace retracer;

static bool printHeaderInfo = false;
static bool printHeaderJson = false;

static void
usage(const char *argv0) {
    fprintf(stderr,
        "Usage: %s [OPTION] <path_to_trace_file>\n"
        "Version: " PATRACE_VERSION "\n"
        "Replay TRACE.\n"
        "\n"
        "  -tid THREADID the function calls invoked by thread <THREADID> will be retraced\n"
        "  -s CALL_SET take snapshot for the calls in the specific call set. Please try to post process the captured snapshot with imagemagick to turn off alpha value if it shows black.\n"
        "  -snapshotprefix PREFIX Prepend this label to every snapshot. Useful for automation.\n"
        "  -step use F1-F4 to step forward frame by frame, F5-F8 to step forward draw call by draw call (not supported on all platforms)\n"
        "  -ores W H override the resolution of the final onscreen rendering (FBOs used in earlier renderpasses are not affected!)\n"
        "  -msaa SAMPLES enable multi sample anti alias\n"
        "  -preload START STOP preload the trace file frames from START to STOP. START must be greater than zero.\n"
        "  -framerange FRAME_START FRAME_END start fps timer at frame start (inclusive), stop timer and playback before frame end (exclusive).\n"
        "  -loop TIMES repeat the preloaded frames at least the given number of times\n"
        "  -looptime SECONDS repeat the preloaded frames at least the given number of seconds\n"
        "  -jsonParameters FILE RESULT_FILE TRACE_DIR path to a JSON file containing the parameters, the output result file and base trace path\n"
        "  -info Show default EGL Config for playback (stored in trace file header). Do not play trace.\n"
        "  -instr Output the supported instrumentation modes as a JSON file. Do not play trace.\n"
        "  -offscreen Run in offscreen mode\n"
        "  -singlewindow Force everything to render in a single window\n"
        "  -singleframe Draw only one frame for each buffer swap (offscreen only)\n"
        "  -skipwork WARMUP_FRAMES Discard GPU work outside frame range with given number of warmup frames. Requires GLES3.\n"
        "  -debug output debug messages\n"
        "  -debugfull output all of the current invoked gl functions, with callNo, frameNo and skipped or discarded information\n"
        "  -infojson Dump the header of the trace file in json format, then exit\n"
        "  -callstats output call statistics to callstats.csv on disk, including the calling number and running time\n"
        "  -overrideEGL Red Green Blue Alpha Depth Stencil, example: overrideEGL 5 6 5 0 16 8, for 16 bit color and 16 bit depth and 8 bit stencil\n"
        "  -strict Use strict EGL mode (fail unless the specified EGL configuration is valid)\n"
        "  -strictcolor Same as -strict, but only checks color channels (RGBA). Useful for dumping when we want to be sure returned EGL is same as requested\n"
        "  -skip CALL_SET skip calls in the specific call set\n"
        "  -collect Collect performance counters\n"
        "  -perfmon Collect performance counters in the built-in perfmon interface\n"
        "  -flush Before starting running the defined measurement range, make sure we flush all pending driver work\n"
        "  -multithread Run all threads in the trace\n"
        "  -shadercache FILENAME Save and load shaders to this cache FILE. Will add .bin and .idx to the given file name.\n"
        "  -strictshadercache Abort if a wanted shader was not found in the shader cache file.\n"
#ifndef __APPLE__
        "  -perf START END run Linux perf on selected frame range and save it to disk\n"
        "  -perfpath PATH Set path to perf binary\n"
        "  -perffreq FREQ Set frequency for perf\n"
        "  -perfout FILENAME Set output filename for perf\n"
#endif
        "  -forceanisolevel LEVEL force all anisotropic filtering levels above 1 to this level\n"
        "  -noscreen Render without visual output (using pbuffer render target)\n"
        "  -singlesurface SURFACE Render all surfaces except the given one to pbuffer render targets instead\n"
        "  -flushonswap Call explicit flush before every call to swap the backbuffer\n"
        "  -cpumask Set explicit CPU mask (written as a string of ones and zeroes)\n"
        "  -libEGL_path=<path.to.libEGL.so>\n"
        "  -libGLESv1_path=<path.to.libGLESv1_CM.so>\n"
        "  -libGLESv2_path=<path.to.libGLESv2.so>\n"
        "  -version Output the version of this program\n"
        "\n"
        "  CALL_SET = interval ( '/' frequency )\n"
        "  interval = '*' | number | start_number '-' end_number\n"
        "  frequency = divisor | \"frame\" | \"draw\"\n"
        , argv0);
}

int readValidValue(char* v)
{
    char* endptr;
    errno = 0;
    int val = strtol(v, &endptr, 10);
    if(errno) {
        perror("strtol");
        exit(1);
    }
    if(endptr == v || *endptr != '\0') {
        fprintf(stderr, "Invalid parameter value: %s\n", v);
        exit(1);
    }

    return val;
}

static bool printInstr()
{
    Json::Value input;
    Collection c(input);
    Json::Value value;
    std::vector<std::string> instrumentations = c.available();
    for (const std::string& s : instrumentations)
    {
        value["instrumentation"].append(s);
    }
    std::cout << value.toStyledString();
    return true;
}

bool ParseCommandLine(int argc, char** argv, RetraceOptions& mOptions)
{
    bool streamline_collector = false;

    // Parse all except first (executable name)
    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];

        // Assume last arg is filename
        if (i==argc-1 && arg[0] != '-') {
            mOptions.mFileName = arg;
            break;
        }

        if (!strcmp(arg, "-tid")) {
            const int tid = readValidValue(argv[++i]);
            if (tid >= PATRACE_THREAD_LIMIT)
            {
                DBG_LOG("Error: thread IDs only up to %d supported (configured thread ID %d).\n", PATRACE_THREAD_LIMIT, tid);
                return false;
            }
            DBG_LOG("Overriding default tid with: %d (warning: this is for testing purposes only!)\n", tid);
            mOptions.mRetraceTid = tid;
        } else if (!strcmp(arg, "-winsize")) {
            DBG_LOG("WARNING: This option is only useful for very old trace files!");
            mOptions.mWindowWidth = readValidValue(argv[++i]);
            mOptions.mWindowHeight = readValidValue(argv[++i]);
            DBG_LOG("Overriding default winsize with %dx%d\n", mOptions.mWindowWidth, mOptions.mWindowHeight);
        } else if (!strcmp(arg, "-ores")) {
            mOptions.mDoOverrideResolution = true;
            mOptions.mOverrideResW = readValidValue(argv[++i]);
            mOptions.mOverrideResH = readValidValue(argv[++i]);
            mOptions.mOverrideResRatioW = mOptions.mOverrideResW / (float) mOptions.mWindowWidth;
            mOptions.mOverrideResRatioH = mOptions.mOverrideResH / (float) mOptions.mWindowHeight;
            DBG_LOG("Override resolution enabled: %dx%d\n",
                    mOptions.mOverrideResW, mOptions.mOverrideResH);
            DBG_LOG("Override resolution ratio: %.2f x %.2f\n",
                    mOptions.mOverrideResRatioW, mOptions.mOverrideResRatioH);
            DBG_LOG("WARNING: Please think twice before using -ores - it is usually a mistake!\n");
        } else if (!strcmp(arg, "-msaa")) {
            mOptions.mOverrideConfig.msaa_samples = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-skipwork")) {
            mOptions.mSkipWork = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-callstats")) {
            mOptions.mCallStats = true;
        } else if (!strcmp(arg, "-perf")) {
            mOptions.mPerfStart = readValidValue(argv[++i]);
            mOptions.mPerfStop = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-perfpath")) {
            mOptions.mPerfPath = argv[++i];
        } else if (!strcmp(arg, "-perfout")) {
            mOptions.mPerfOut = argv[++i];
        } else if (!strcmp(arg, "-perffreq")) {
            mOptions.mPerfFreq = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-s")) {
            mOptions.mSnapshotCallSet = new common::CallSet(argv[++i]);
        } else if (!strcmp(arg, "-framenamesnaps")) {
            mOptions.mSnapshotFrameNames = true;
        } else if (!strcmp(arg, "-snapshotprefix")) {
            mOptions.mSnapshotPrefix = argv[++i];
        } else if (!strcmp(arg, "-forceanisolevel")) {
            mOptions.mForceAnisotropicLevel = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-step")) {
            mOptions.mStepMode = true;
        } else if (!strcmp(arg, "--help") || !strcmp(arg, "-h")) {
            usage(argv[0]);
            return false;
        } else if (!strcmp(arg, "-cpumask")) {
            mOptions.mCpuMask = argv[++i];
        } else if (!strcmp(arg, "-loop")) {
            mOptions.mLoopTimes = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-looptime")) {
            mOptions.mLoopSeconds = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-framerange")) {
            mOptions.mBeginMeasureFrame = readValidValue(argv[++i]);
            mOptions.mEndMeasureFrame = readValidValue(argv[++i]);
            if (mOptions.mBeginMeasureFrame >= mOptions.mEndMeasureFrame)
            {
                DBG_LOG("Start frame must be lower than end frame. (End frame is never played.)\n");
                return false;
            }
        } else if (!strcmp(arg, "-preload")) {
            mOptions.mPreload = true;
            mOptions.mBeginMeasureFrame = readValidValue(argv[++i]);
            mOptions.mEndMeasureFrame = readValidValue(argv[++i]);
            if (mOptions.mBeginMeasureFrame >= mOptions.mEndMeasureFrame)
            {
                DBG_LOG("Start frame must be lower than end frame. (End frame is never played.)\n");
                return false;
            }
        } else if (!strcmp(arg, "-jsonParameters")) {
            const char *jsonParameters = argv[++i];
            const char *resultFile = argv[++i];
            const char *traceDir = argv[++i];
            std::ifstream t(jsonParameters);
            std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
            TraceExecutor::initFromJson(str, traceDir, resultFile);
        } else if (!strcmp(arg, "-info")) {
            printHeaderInfo = true;
        } else if (!strcmp(arg, "-debug")) {
            mOptions.mDebug = 1;
        } else if (!strcmp(arg, "-debugfull")) {
            mOptions.mDebug = 2;
        } else if (!strcmp(arg, "-statelog")) {
            mOptions.mStateLogging = true;
        } else if (!strcmp(arg, "-noscreen")) {
            mOptions.mPbufferRendering = true;
        } else if (!strcmp(arg, "-singlesurface")) {
            mOptions.mSingleSurface = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-perfmon")) {
            mOptions.mPerfmon = true;
        } else if (!strcmp(arg, "-collect")) {
            if (!gRetracer.mCollectors) gRetracer.mCollectors = new Collection(Json::Value());
        } else if (!strcmp(arg, "-collect_streamline")) {
            if (!gRetracer.mCollectors) gRetracer.mCollectors = new Collection(Json::Value());
            streamline_collector = true;
        } else if (!strcmp(arg, "-flushonswap")) {
            mOptions.mFinishBeforeSwap = true;
        } else if (!strcmp(arg, "-flush")) {
            mOptions.mFlushWork = true;
        } else if (!strcmp(arg, "-infojson")) {
            printHeaderInfo = true;
            printHeaderJson = true;
        } else if (!strcmp(arg, "-offscreen")) {
            mOptions.mForceOffscreen = true;
            // When running offscreen, force onscreen EGL to most compatible mode known: 5650 00
            mOptions.mOnscreenConfig = EglConfigInfo(5, 6, 5, 0, 0, 0, 0, 0);
        } else if (!strcmp(arg, "-singlewindow")) {
            mOptions.mForceSingleWindow = true;
        } else if (!strcmp(arg, "-multithread")) {
            mOptions.mMultiThread = true;
        } else if (!strcmp(arg, "-shadercache")) {
            mOptions.mShaderCacheFile = argv[++i];
        } else if(!strcmp(arg, "-strictshadercache")) {
            mOptions.mShaderCacheRequired = true;
        } else if (!strcmp(arg, "-insequence")) {
            // nothing, this is always the case now
        } else if (!strcmp(arg, "-singleframe")) {
            // Draw offscreen using 1 big tile
            mOptions.mOnscrSampleH *= 10;
            mOptions.mOnscrSampleW *= 10;
            mOptions.mOnscrSampleNumX = 1;
            mOptions.mOnscrSampleNumY = 1;
        } else if (!strcmp(arg, "-overrideEGL")) {
            mOptions.mOverrideConfig.red = readValidValue(argv[++i]);
            mOptions.mOverrideConfig.green = readValidValue(argv[++i]);
            mOptions.mOverrideConfig.blue = readValidValue(argv[++i]);
            mOptions.mOverrideConfig.alpha = readValidValue(argv[++i]);
            mOptions.mOverrideConfig.depth = readValidValue(argv[++i]);
            mOptions.mOverrideConfig.stencil = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-strict")) {
            mOptions.mStrictEGLMode = true;
        } else if (!strcmp(arg, "-strictcolor")) {
            mOptions.mStrictColorMode = true;
        } else if (!strcmp(arg, "-skip")) {
            mOptions.mSkipCallSet = new common::CallSet(argv[++i]);
        } else if (strstr(arg, "-lib")) {
            const char* strEGL = "-libEGL_path=";
            const char* strGLES1 = "-libGLESv1_path=";
            const char* strGLES2 = "-libGLESv2_path=";

            if (strstr(arg, strEGL)) {
                std::string libPath = std::string(arg).substr( strlen(strEGL), strlen(arg) - strlen(strEGL) );
                SetCommandLineEGLPath(libPath);
            } else if (strstr(arg, strGLES1)) {
                std::string libPath = std::string(arg).substr( strlen(strGLES1), strlen(arg) - strlen(strGLES1) );
                SetCommandLineGLES2Path(libPath);
            } else if (strstr(arg, strGLES2)) {
                std::string libPath = std::string(arg).substr( strlen(strGLES2), strlen(arg) - strlen(strGLES2) );
                SetCommandLineGLES2Path(libPath);
            } else{
                DBG_LOG("error: unknown option %s\n", arg);
                usage(argv[0]);
                return false;
            }
        } else if(!strcmp(arg, "-instr")) {
            bool succ = printInstr();
            exit(succ ? 0 : 1);
        } else if (!strcmp(arg, "-version")) {
            std::cout << "Version: " PATRACE_VERSION << std::endl;
            exit(0);
        } else if (!strcmp(arg, "-dmasharedmem")) {
            mOptions.dmaSharedMemory = true;
        } else {
            DBG_LOG("error: unknown option %s\n", arg);
            usage(argv[0]);
            return false;
        }
    }

    if (mOptions.mSingleSurface != -1 && mOptions.mForceSingleWindow)
    {
        DBG_LOG("Single surface and single window options cannot be combined!\n");
        return false;
    }
    if (mOptions.mLoopTimes && !mOptions.mPreload)
    {
        DBG_LOG("Loop option requires preload\n");
        return false;
    }

    if (gRetracer.mCollectors)
    {
        std::vector<std::string> collectors = gRetracer.mCollectors->available();
        std::vector<std::string> filtered;
        for (const std::string& s : collectors)
        {
            if (s == "rusage" || s == "gpufreq" || s == "procfs" || s == "cpufreq" || s == "perf")
            {
                filtered.push_back(s);
            }
            else if (s == "streamline" && streamline_collector)
            {
                filtered.push_back(s);
                DBG_LOG("Streamline integration support enabled\n");
            }
        }
        if (!gRetracer.mCollectors->initialize(filtered))
        {
            fprintf(stderr, "Failed to initialize collectors\n");
        }
        else DBG_LOG("libcollector instrumentation enabled through cmd line.\n");
    }
    return true;
}

extern "C"
int main(int argc, char** argv)
{
    if (!ParseCommandLine(argc, argv, gRetracer.mOptions))
    {
        return 1;
    }

    if (gRetracer.mOptions.mFileName.empty())
    {
        std::cerr << "No trace file name specified.\n";
        usage(argv[0]);
        return 1;
    }

    if (printHeaderInfo)
    {
        common::InFile file;

        if (!file.Open(gRetracer.mOptions.mFileName.c_str(), true))
        {
            return 1;
        }

        if (printHeaderJson)
        {
            std::string js = file.getJSONHeaderAsString(true);
            std::cout << js << std::endl;
        }
        else
        {
            file.printHeaderInfo();
        }

        return 0;
    }

    // Register Entries before opening tracefile as sigbook is read there
    common::gApiInfo.RegisterEntries(gles_callbacks);
    common::gApiInfo.RegisterEntries(egl_callbacks);

    if (!gRetracer.OpenTraceFile(gRetracer.mOptions.mFileName.c_str()))
    {
        DBG_LOG("Failed to open %s\n", gRetracer.mOptions.mFileName.c_str() );
        return -1;
    }

    // 3. init egl and gles, using final combination of settings (header + override)
    GLWS::instance().Init(gRetracer.mOptions.mApiVersion);

    if (gRetracer.mOptions.mStepMode)
    {
        if (!GLWS::instance().steppable())
        {
            DBG_LOG("Step mode not supported on this platform!\n");
            return -1;
        }
        gRetracer.frameBudget = 0;
        gRetracer.drawBudget = 1; // we need at least one draw to get things started
    }
    gRetracer.Retrace();
    GLWS::instance().Cleanup();
    return 0;
}
