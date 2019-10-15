#include <errno.h>

#include <retracer/retracer.hpp>
#include <retracer/glws.hpp>
#include <retracer/trace_executor.hpp>
#include <retracer/retrace_api.hpp>
#include <retracer/config.hpp>
#include <dispatch/eglproc_retrace.hpp>

#include "libcollector/interface.hpp"

#include "cmd_options.hpp"
#include "helper/states.h"

using namespace retracer;

static void
usage(const char *argv0) {
    fprintf(stderr,
        "Usage: %s [OPTION] <path_to_trace_file>\n"
        "Version: " PATRACE_VERSION "\n"
        "Replay TRACE.\n"
        "\n"
        "  -tid THREADID the function calls invoked by thread <THREADID> will be retraced\n"
        "  -s CALL_SET take snapshot for the calls in the specific call set. Please try to post process the captured snapshot with imagemagick to turn off alpha value if it shows black.\n"
        "  -step use F1-F4 to step forward frame by frame, F5-F8 to step forward draw call by draw call (only supported on desktop linux)\n"
        "  -ores W H override resolution of onscreen rendering (FBO's are not affected)\n"
        "  -msaa SAMPLES enable multi sample anti alias\n"
        "  -preload START STOP preload the trace file frames from START to STOP. START must be greater than zero.\n"
        "  -framerange FRAME_START FRAME_END, start fps timer at frame start (inclusive), stop timer and playback before frame end (exclusive).\n"
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
        "  -flush Before starting running the defined measurement range, make sure we flush all pending driver work\n"
        "  -multithread Run all threads in the trace\n"
#ifndef __APPLE__
        "  -perf START END run Linux perf on selected frame range and save it to disk\n"
        "  -perfpath PATH Set path to perf binary\n"
        "  -perffreq FREQ Set frequency for perf\n"
        "  -perfout FILENAME Set output filename for perf\n"
#endif
        "  -noscreen Render without visual output (using pbuffer render target)\n"
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

static bool printInstr(Json::Value input)
{
    /* TODO: read configuration from somewhere. */
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

bool ParseCommandLine(int argc, char** argv, CmdOptions& cmdOpts)
{
    // Parse all except first (executable name)
    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        // Assume last arg is filename
        if (i==argc-1 && arg[0] != '-') {
            cmdOpts.fileName = arg;
            break;
        }

        if (!strcmp(arg, "-tid")) {
            cmdOpts.tid = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-winsize")) {
            cmdOpts.winW = readValidValue(argv[++i]);
            cmdOpts.winH = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-ores")) {
            cmdOpts.oresW = readValidValue(argv[++i]);
            cmdOpts.oresH = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-msaa")) {
            cmdOpts.eglConfig.msaa_samples = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-skipwork")) {
            cmdOpts.skipWork = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-dumpstatic")) {
            cmdOpts.dumpStatic = true;
        } else if (!strcmp(arg, "-callstats")) {
            cmdOpts.callStats = true;
        } else if (!strcmp(arg, "-perf")) {
            cmdOpts.perfStart = readValidValue(argv[++i]);
            cmdOpts.perfStop = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-perfpath")) {
            cmdOpts.perfPath = argv[++i];
        } else if (!strcmp(arg, "-perfout")) {
            cmdOpts.perfOut = argv[++i];
        } else if (!strcmp(arg, "-perffreq")) {
            cmdOpts.perfFreq = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-s")) {
            cmdOpts.snapshotCallSet = new common::CallSet(argv[++i]);
        } else if (!strcmp(arg, "-framenamesnaps")) {
            cmdOpts.snapshotFrameNames = true;
        } else if (!strcmp(arg, "-step")) {
            cmdOpts.stepMode = true;
        } else if (!strcmp(arg, "--help") || !strcmp(arg, "-h")) {
            usage(argv[0]);
            return false;
        } else if (!strcmp(arg, "-cpumask")) {
            cmdOpts.cpuMask = argv[++i];
        } else if (!strcmp(arg, "-framerange")) {
            cmdOpts.beginMeasureFrame = readValidValue(argv[++i]);
            cmdOpts.endMeasureFrame = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-preload")) {
            cmdOpts.preload = true;
            cmdOpts.beginMeasureFrame = readValidValue(argv[++i]);
            cmdOpts.endMeasureFrame = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-jsonParameters")) {
            // ignore here
        } else if (!strcmp(arg, "-info")) {
            cmdOpts.printHeaderInfo = true;
        } else if (!strcmp(arg, "-debug")) {
            cmdOpts.debug = 1;
        } else if (!strcmp(arg, "-debugfull")) {
            cmdOpts.debug = 2;
        } else if (!strcmp(arg, "-statelog")) {
            cmdOpts.stateLogging = true;
        } else if (!strcmp(arg, "-noscreen")) {
            cmdOpts.pbufferRendering = true;
        } else if (!strcmp(arg, "-collect")) {
            cmdOpts.collectors = true;
        } else if (!strcmp(arg, "-collect_streamline")) {
            cmdOpts.collectors_streamline = true;
        } else if (!strcmp(arg, "-flushonswap")) {
            cmdOpts.finishBeforeSwap = true;
        } else if (!strcmp(arg, "-flush")) {
            cmdOpts.flushWork = true;
        } else if (!strcmp(arg, "-drawlog")) {
            cmdOpts.drawLogging = true;
            stateLoggingEnabled = true;
        } else if (!strcmp(arg, "-infojson")) {
            cmdOpts.printHeaderInfo = true;
            cmdOpts.printHeaderJson = true;
        } else if (!strcmp(arg, "-offscreen")) {
            cmdOpts.forceOffscreen = true;
        } else if (!strcmp(arg, "-singlewindow")) {
            cmdOpts.forceSingleWindow = true;
        } else if (!strcmp(arg, "-multithread")) {
            cmdOpts.multiThread = true;
        } else if (!strcmp(arg, "-insequence")) {
            // nothing, this is always the case now
        } else if (!strcmp(arg, "-singleframe")) {
            cmdOpts.singleFrameOffscreen = true;
        } else if (!strcmp(arg, "-overrideEGL")) {
            cmdOpts.eglConfig.red = readValidValue(argv[++i]);
            cmdOpts.eglConfig.green = readValidValue(argv[++i]);
            cmdOpts.eglConfig.blue = readValidValue(argv[++i]);
            cmdOpts.eglConfig.alpha = readValidValue(argv[++i]);
            cmdOpts.eglConfig.depth = readValidValue(argv[++i]);
            cmdOpts.eglConfig.stencil = readValidValue(argv[++i]);
        } else if (!strcmp(arg, "-strict")) {
            cmdOpts.strictEGLMode = true;
        } else if (!strcmp(arg, "-strictcolor")) {
            cmdOpts.strictColorMode = true;
        } else if (!strcmp(arg, "-skip")) {
            cmdOpts.skipCallSet = new common::CallSet(argv[++i]);
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
            bool succ = printInstr(Json::Value()); // TBD, pass in JSON from --jsonParameters
            exit(succ ? 0 : 1);
        } else if (!strcmp(arg, "-version")) {
            std::cout << "Version: " PATRACE_VERSION << std::endl;
            exit(0);
        } else {
            DBG_LOG("error: unknown option %s\n", arg);
            usage(argv[0]);
            return false;
        }
    }

    return true;
}

bool useJsonParameters(int argc, char** argv, const char** jsonParameters, const char** resultFile, const char** traceDir)
{
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-jsonParameters")) {
            *jsonParameters = argv[++i];
            *resultFile = argv[++i];
            *traceDir = argv[++i];
            DBG_LOG("JSON: %s - result file: %s - trace dir: %s\n", *jsonParameters, *resultFile, *traceDir);
            return true;
        }
    }

    return false;
}

extern "C"
int main(int argc, char** argv)
{
    const char* jsonParameters = NULL;
    const char* resultFile = NULL;
    const char* traceDir = NULL;
    bool useJson = useJsonParameters(argc, argv, &jsonParameters, &resultFile, &traceDir);

    if (useJson)
    {
        /* if JSON file specified, no need to read other command line parameters as
         * only parameters from JSON will be used */
        std::ifstream t(jsonParameters);
        std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        TraceExecutor::initFromJson(str, std::string(traceDir), std::string(resultFile));
        GLWS::instance().Init();
        gRetracer.Retrace();
        return 0;
    } else {
        CmdOptions cmdOptions;
        if(!ParseCommandLine(argc, argv, cmdOptions)) {
            return 1;
        }

        if (cmdOptions.dumpStatic)
        {
            remove("static.json"); // cannot risk making corrupt data
        }

        if(cmdOptions.fileName.empty()) {
            std::cerr << "No trace file name specified.\n";
            usage(argv[0]);
            return 1;
        }

        if(cmdOptions.printHeaderInfo) {
            common::InFile file;
            if ( !file.Open( cmdOptions.fileName.c_str(), true )){
                return 1;
            }
            if ( cmdOptions.printHeaderJson ) {
                std::string js = file.getJSONHeaderAsString(true);
                std::cout << js << std::endl;
            } else {
                file.printHeaderInfo();
            }

            return 0;
        }

        // Register Entries before opening tracefile as sigbook is read there
        common::gApiInfo.RegisterEntries(gles_callbacks);
        common::gApiInfo.RegisterEntries(egl_callbacks);

        // 1. Load defaults from file
        if ( !gRetracer.OpenTraceFile( cmdOptions.fileName.c_str() )) {
            DBG_LOG("Failed to open %s\n", cmdOptions.fileName.c_str() );
            return 1;
        }

        // 2. Now that tracefile is opened and defaults loaded, override
        if ( !gRetracer.overrideWithCmdOptions( cmdOptions ) ) {
            DBG_LOG("Failed to override Cmd Options\n");
            return -1;
        }

        // 3. init egl and gles, using final combination of settings (header + override)
        GLWS::instance().Init(gRetracer.mOptions.mApiVersion);

        if (cmdOptions.stepMode)
        {
            // Keep forward until EGL context & drawable have been created
            retracer::Drawable *cd = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getDrawable();
            while (cd == NULL)
            {
                bool success = gRetracer.RetraceForward(1, 0);
                if (!success)
                {
                    DBG_LOG("Nothing found to render!\n");
                    return -2;
                }
                cd = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getDrawable();
            }

            if (cd)
            {
                GLWS::instance().processStepEvent();
            }
        }
        else
        {
            gRetracer.Retrace();
        }

    }
    GLWS::instance().Cleanup();
    return 0;
}
