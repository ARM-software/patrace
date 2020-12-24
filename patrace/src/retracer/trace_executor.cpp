#include "trace_executor.hpp"

#include "retracer.hpp"
#include <retracer/glws.hpp>

#include <retracer/retrace_api.hpp>
#include "retracer/value_map.hpp"

#include "jsoncpp/include/json/writer.h"
#include "jsoncpp/include/json/reader.h"

#include <limits.h>
#include <zlib.h>
#include <errno.h>

#include "common/base64.hpp"
#include "common/os_string.hpp"
#include "common/trace_callset.hpp"

#include "libcollector/interface.hpp"

#include <sstream>
#include <vector>
#include <sys/stat.h>

#ifdef ANDROID
#include <android/log.h>
#endif

const char* TraceExecutor::ErrorNames[] = {
            "TRACE_ERROR_FILE_NOT_FOUND",
            "TRACE_ERROR_INVALID_JSON",
            "TRACE_ERROR_INVALID_PARAMETER",
            "TRACE_ERROR_MISSING_PARAMETER",
            "TRACE_ERROR_PARAMETER_OUT_OF_BOUNDS",
            "TRACE_ERROR_OUT_OF_MEMORY",
            "TRACE_ERROR_MEMORY_BUDGET",
            "TRACE_ERROR_INITIALISING_INSTRUMENTATION",
            "TRACE_ERROR_CAPTURING_INSTRUMENTATION_DATA",
            "TRACE_ERROR_INCONSISTENT_TRACE_FILE",
            "TRACE_ERROR_GENERIC"
        };
static_assert(sizeof(TraceExecutor::ErrorNames)/sizeof(TraceExecutor::ErrorNames[0]) == TRACE_ERROR_COUNT,
        "Enum-to-string table length mismatch");

std::string TraceExecutor::mResultFile;
std::vector<TraceErrorType> TraceExecutor::mErrorList;
ProgramAttributeListMap_t TraceExecutor::mProgramAttributeListMap;
ProgramInfoList_t TraceExecutor::mProgramInfoList;

using namespace retracer;

void TraceExecutor::overrideDefaultsWithJson(Json::Value &value)
{
    retracer::RetraceOptions& options = gRetracer.mOptions;

    options.mDoOverrideResolution = value.get("overrideResolution", false).asBool();
    options.mOverrideResW = value.get("overrideWidth", -1).asInt();
    options.mOverrideResH = value.get("overrideHeight", -1).asInt();
    options.mFailOnShaderError = value.get("overrideFailOnShaderError", options.mFailOnShaderError).asBool();
    options.mLoopTimes = value.get("loopTimes", options.mLoopTimes).asInt();
    if (value.isMember("loopSeconds"))
    {
        options.mLoopSeconds = value["loopSeconds"].asInt();
    }
    options.mCallStats = value.get("callStats", options.mCallStats).asBool();
    if (options.mCallStats)
    {
        DBG_LOG("Callstats output enabled\n");
    }

    if (value.get("finishBeforeSwap", false).asBool())
    {
        options.mFinishBeforeSwap = true;
    }

    if (value.get("flushWork", false).asBool())
    {
        options.mFlushWork = true;
    }

    if (options.mDoOverrideResolution && (options.mOverrideResW < 0 || options.mOverrideResH < 0))
    {
        gRetracer.reportAndAbort("Missing actual resolution when resolution override set");
    }
    if (options.mDoOverrideResolution)
    {
        options.mOverrideResRatioW = options.mOverrideResW / (float) options.mWindowWidth;
        options.mOverrideResRatioH = options.mOverrideResH / (float) options.mWindowHeight;
    }

    options.mForceAnisotropicLevel = value.get("forceAnisotropicLevel", options.mForceAnisotropicLevel).asInt();

    int jsWidth = value.get("width", -1).asInt();
    int jsHeight = value.get("height", -1).asInt();
    if (gRetracer.mFile.getHeaderVersion() >= common::HEADER_VERSION_2
        && (jsWidth != options.mWindowWidth || jsHeight != options.mWindowHeight)
        && (jsWidth > 0 && jsHeight > 0))
    {
        DBG_LOG("Wrong window size specified, must be same as in trace header. This option is only useful for very old trace files!");
        jsWidth = -1;
        jsHeight = -1;
    }
    if (jsWidth != -1 && jsHeight != -1)
    {
        DBG_LOG("Changing window size from (%d, %d) to (%d, %d)\n", options.mWindowWidth, options.mWindowHeight, jsWidth, jsHeight);
        options.mWindowWidth = jsWidth;
        options.mWindowHeight = jsHeight;
    }

    EglConfigInfo eglConfig(
        value.get("colorBitsRed", -1).asInt(),
        value.get("colorBitsGreen", -1).asInt(),
        value.get("colorBitsBlue", -1).asInt(),
        value.get("colorBitsAlpha", -1).asInt(),
        value.get("depthBits", -1).asInt(),
        value.get("stencilBits", -1).asInt(),
        value.get("msaaSamples", -1).asInt(),
        0
    );

    if (value.isMember("cpumask"))
    {
        options.mCpuMask = value.get("cpumask", "").asString();
    }

    options.mForceSingleWindow = value.get("forceSingleWindow", options.mForceSingleWindow).asBool();
    options.mForceOffscreen = value.get("offscreen", options.mForceOffscreen).asBool();
    options.mPbufferRendering = value.get("noscreen", options.mPbufferRendering).asBool();
    options.mSingleSurface = value.get("singlesurface", options.mSingleSurface).asInt();
    if (value.isMember("skipWork"))
    {
        options.mSkipWork = value.get("skipWork", -1).asInt();
    }

    options.mOverrideConfig = eglConfig;
    options.mMeasurePerFrame = value.get("measurePerFrame", false).asBool();

    if (value.isMember("frames")) {
        std::string frames = value.get("frames", "").asString();
        DBG_LOG("Frame string: %s\n", frames.c_str());
        unsigned int start, end;
        if (sscanf(frames.c_str(), "%u-%u", &start, &end) == 2)
        {
            if (start >= end)
            {
                gRetracer.reportAndAbort("Start frame must be lower than end frame. (End frame is never played.)");
            }
            options.mBeginMeasureFrame = start;
            options.mEndMeasureFrame = end;
        } else {
            gRetracer.reportAndAbort("Invalid frames parameter [ %s ]", frames.c_str());
        };
    }

    options.mPreload = value.get("preload", false).asBool();

    // Values needed by CLI and GUI
    options.mSnapshotPrefix = value.get("snapshotPrefix", "").asString();

    if (options.mSnapshotPrefix.compare("*") == 0)
    {
        // Create temporary directory to store snapshots in
#if defined(ANDROID)
        std::string snapsDir = "/sdcard/apitrace/retracer-snaps/";
        system("rm -rf /sdcard/apitrace/retracer-snaps/");
        system("mkdir -p /sdcard/apitrace/retracer-snaps");
#elif defined(__APPLE__)
        std::string snapsDir = "/tmp/retracer-snaps/";
        // Can't use system() on iOS
        mkdir(snapsDir.c_str(), 0777);
#else // fbdev / desktop
        std::string snapsDir = "/tmp/retracer-snaps/";
        int sysRet = system("rm -rf /tmp/retracer-snaps/");
        sysRet += system("mkdir -p /tmp/retracer-snaps/");
        if (sysRet != 0)
        {
            DBG_LOG("Failed to prepare directory: %s\n", snapsDir.c_str());
        }
#endif
        options.mSnapshotPrefix = snapsDir;
    }

    options.mSnapshotFrameNames = value.get("snapshotFrameNames", false).asBool();

    // Whether or not to upload taken snapshots.
    options.mUploadSnapshots = value.get("snapshotUpload", false).asBool();

    if (value.isMember("snapshotCallset")) {
        DBG_LOG("snapshotCallset = %s\n", value.get("snapshotCallset", "").asCString());
        delete options.mSnapshotCallSet;
        options.mSnapshotCallSet = new common::CallSet( value.get("snapshotCallset", "").asCString() );
    }

    options.mStateLogging = value.get("statelog", false).asBool();
    stateLoggingEnabled = value.get("drawlog", false).asBool();
    options.mDebug = (int)value.get("debug", false).asBool();
    if (options.mDebug)
    {
        DBG_LOG("Debug mode enabled.\n");
    }
    options.mStoreProgramInformation = value.get("storeProgramInformation", false).asBool();
    options.mRemoveUnusedVertexAttributes = value.get("removeUnusedVertexAttributes", false).asBool();

    if (value.get("offscreenBigTiles", false).asBool())
    {
        // Draw offscreen using 4 big tiles, so that their contents are easily visible
        options.mOnscrSampleH *= 12;
        options.mOnscrSampleW *= 12;
        options.mOnscrSampleNumX = 2;
        options.mOnscrSampleNumY = 2;
    }
    else if (value.get("offscreenSingleTile", false).asBool())
    {
        // Draw offscreen using 1 big tile
        options.mOnscrSampleH *= 10;
        options.mOnscrSampleW *= 10;
        options.mOnscrSampleNumX = 1;
        options.mOnscrSampleNumY = 1;
    }

    if (value.get("multithread", false).asBool())
    {
        options.mMultiThread = true;
    }

    if (value.isMember("instrumentation"))
    {
        DBG_LOG("Legacy instrumentation support requested -- fix your JSON! Translating...\n");
        Json::Value legacy;
        for (const Json::Value& v : value["instrumentation"])
        {
            Json::Value emptyDict;
            legacy[v.asString()] = emptyDict;
        }
        gRetracer.mCollectors = new Collection(legacy);
        gRetracer.mCollectors->initialize();
    }

    if (value.isMember("collectors"))
    {
        gRetracer.mCollectors = new Collection(value["collectors"]);
        gRetracer.mCollectors->initialize();
        DBG_LOG("libcollector instrumentation enabled through JSON.\n");
    }

    if (value.get("dmaSharedMem", false).asBool())
    {
        options.dmaSharedMemory = true;
    }

    if (value.get("perfmon", false).asBool())
    {
        options.mPerfmon = true;
    }

    if (value.isMember("shaderCache"))
    {
        options.mShaderCacheFile = value.get("shaderCache", "").asString();
    }
    if (value.isMember("strictShaderCache"))
    {
        options.mShaderCacheRequired = value.get("strictShaderCache", false).asBool();
    }

    DBG_LOG("Thread: %d - override: %s (%d, %d)\n",
            options.mRetraceTid, options.mDoOverrideResolution ? "Yes" : "No", options.mOverrideResW, options.mOverrideResH);
}

/**
 Inits the global retracer object from data provided in JSON format.

 @param json_data Parameters in json format.
 @param trace_dir The directory containing the trace file.
 @param result_file The path where the result should be written.
 @return Returns true if successful. If false is returned, an error might be written to the result file.
 */
void TraceExecutor::initFromJson(const std::string& json_data, const std::string& trace_dir, const std::string& result_file)
{
    mResultFile = result_file;

    /*
     * The order is important here:
     *
     * 1. Read trace filename from JSON
     * 2. Set up function pointer entries.
     * 3. Open tracefile and read header defaults
     * 4. Override header defaults with options from the JSON structure + other config like instrumentation.
     */

    Json::Value value;
    Json::Reader reader;
    if (!reader.parse(json_data, value))
    {
        gRetracer.reportAndAbort("JSON parse error: %s\n", reader.getFormattedErrorMessages().c_str());
    }

    // A path is absolute if
    // -on Unix, it begins with a slash
    // -on Windows, it begins with (back)slash after chopping of potential drive letter

    if (value.isMember("file"))
    {
        bool pathIsAbsolute = value.get("file", "").asString()[0] == '/';
        std::string traceFilePath;
        if (pathIsAbsolute)
        {
            traceFilePath = value.get("file", "").asString();
        } else {
            traceFilePath = std::string(trace_dir) + "/" + value.get("file", "").asString();
        }
        gRetracer.mOptions.mFileName = traceFilePath;
    }

    // 2. now that defaults are loaded
    overrideDefaultsWithJson(value);

#ifdef ANDROID
    bool claim_memory = value.get("claimMemory", false).asBool();
    if (claim_memory){
        float reserveFactor = 0.95f;
        MemoryInfo::reserveAndReleaseMemory(MemoryInfo::getFreeMemoryRaw() * reserveFactor);
    }
#endif

#ifndef _WIN32
    unsigned int membudget = 0;
    membudget = value.get("membudget", membudget).asInt();

    if (MemoryInfo::getFreeMemory() < ((unsigned long)membudget)*1024*1024) {
        unsigned long diff = ((unsigned long)membudget)*1024*1024 - MemoryInfo::getFreeMemory();
        gRetracer.reportAndAbort("Cannot satisfy required memory budget, lacking %lu MiB. Aborting...\n", diff);
    }
#endif
}

void TraceExecutor::addError(TraceExecutorErrorCode code, const std::string &error_description){
    mErrorList.push_back(TraceErrorType(code, error_description));
}

void TraceExecutor::addDisabledButActiveAttribute(int program, const std::string& attributeName)
{
    ProgramAttributeListMap_t::iterator programIt = mProgramAttributeListMap.find(program);
    if (programIt == mProgramAttributeListMap.end())
    {
        mProgramAttributeListMap[program] = AttributeList_t();
    }

    mProgramAttributeListMap[program].insert(attributeName);
}

Json::Value& TraceExecutor::addProgramInfo(int program, int originalProgramName, retracer::hmap<unsigned int>& shaderRevMap)
{
    ProgramInfo pi(program);
    Json::Value json;
    Json::Value attributeNames(Json::arrayValue);

    for (int i = 0; i < pi.activeAttributes; ++i)
    {
        VertexArrayInfo vai = pi.getActiveAttribute(i);
        attributeNames[i] = vai.name;
    }

    json["callNo"] = gRetracer.GetCurCallId();
    json["activeAttributeNames"] = attributeNames;
    json["activeAttributeCount"] = pi.activeAttributes;
    json["id"] = originalProgramName;
    json["linkStatus"] = pi.linkStatus;
    if (pi.linkStatus == GL_FALSE)
    {
        json["linkLog"] = pi.getInfoLog();

        if(gRetracer.mOptions.mFailOnShaderError) {
            addError(TRACE_ERROR_OUT_OF_MEMORY, "A shader program failed to link"); // TODO Proper error code
            gRetracer.mFailedToLinkShaderProgram = true;
        }
    }

    json["shaders"] = Json::Value(Json::objectValue);
    for (std::vector<unsigned int>::iterator it = pi.shaderNames.begin(); it != pi.shaderNames.end(); ++it)
    {
        ShaderInfo shader(*it);
        unsigned int originalShaderName = shaderRevMap.RValue(shader.id);
        std::stringstream ss;
        ss << originalShaderName;
        std::string id = ss.str();

        json["shaders"][id] = Json::Value(Json::objectValue);
        json["shaders"][id]["compileStatus"] = shader.compileStatus;
        if (shader.compileStatus == GL_FALSE)
        {
            json["shaders"][id]["compileLog"] = shader.getInfoLog();
        }
    }
    mProgramInfoList.push_back(json);
    return mProgramInfoList.back();
}

void TraceExecutor::writeError(TraceExecutorErrorCode error_code, const std::string &error_description)
{
#ifdef ANDROID
#ifdef PLATFORM_64BIT
    __android_log_write(ANDROID_LOG_FATAL, "paretrace64", error_description.c_str());
#else
    __android_log_write(ANDROID_LOG_FATAL, "paretrace32", error_description.c_str());
#endif
#else
    DBG_LOG("%s\n", error_description.c_str());
#endif
    addError(error_code, error_description);
    if (!writeData(Json::Value(), 0, 0.0f))
    {
        DBG_LOG("Failed to output error log!\n");
    }
    clearError();
}

void TraceExecutor::clearResult()
{
    mProgramAttributeListMap.clear();
    mProgramInfoList.clear();
    clearError();
}

void TraceExecutor::clearError()
{
    mErrorList.clear();
}

bool TraceExecutor::writeData(Json::Value result_data_value, int frames, float duration)
{
    Json::Value result_value;
    if (!mErrorList.empty())
    {
        Json::Value error_list_value;
        Json::Value error_list_description;
        for (std::vector<TraceErrorType>::iterator it = mErrorList.begin(); it!=mErrorList.end(); ++it) {
            if (it->error_code < TRACE_ERROR_COUNT)
            {
                error_list_value.append(ErrorNames[it->error_code]);
                error_list_description.append(it->error_description);
            }
        }
        result_value["error"] = error_list_value;
        result_value["error_description"] = error_list_description;
    }
    else if (frames > 0 || duration > 0.0f)
    {
        Json::Value result_list_value;
        if (gRetracer.mCollectors)
        {
            result_data_value["frame_data"] = gRetracer.mCollectors->results();
            try {
                if (result_data_value["frame_data"].isMember("ferret")) {
                    if ( result_data_value["frame_data"]["ferret"].empty() ) {
                        DBG_LOG("Ferret data detected, but the results are empty.\n");
                    } else {
                        DBG_LOG("Ferret data detected, checking for postprocessed results...\n");
                        // Calculate CPU FPS
                        if (result_data_value["frame_data"]["ferret"].isMember("postprocessed_results")) {
                            DBG_LOG("Postprocessed ferret data detected, calculating CPU FPS!\n");

                            int main_thread = result_data_value["frame_data"]["ferret"]["postprocessed_results"]["main_thread_index"].asInt();
                            double main_thread_megacycles = result_data_value["frame_data"]["ferret"]["postprocessed_results"]["main_thread_megacycles"].asInt();
                            DBG_LOG("Main (heaviest) thread index set to: %d, main thread mega cycles consumed = %f\n", main_thread, main_thread_megacycles);

                            result_data_value["main_thread_cpu_runtime@3GHz"] = main_thread_megacycles / 3000.0;

                            double main_thread_cpu_runtime_3ghz = result_data_value["main_thread_cpu_runtime@3GHz"].asDouble();
                            if (main_thread_cpu_runtime_3ghz == 0.0) {
                                DBG_LOG("WARNING: Main thread CPU megacycles reported as 0. Possibly invalid ferret results.\n");
                                result_data_value["cpu_fps_main_thread@3GHz"] = 0.0;
                            } else {
                                result_data_value["cpu_fps_main_thread@3GHz"] = static_cast<double>(frames) / main_thread_cpu_runtime_3ghz;
                            }

                            double total_megacycles = result_data_value["frame_data"]["ferret"]["postprocessed_results"]["megacycles_sum"].asDouble();
                            DBG_LOG("Total CPU mega cycles consumed = %f\n", total_megacycles);

                            result_data_value["full_system_cpu_runtime@3GHz"] = total_megacycles / 3000.0;

                            double full_system_cpu_runtime_3ghz = result_data_value["full_system_cpu_runtime@3GHz"].asDouble();
                            if (full_system_cpu_runtime_3ghz == 0.0) {
                                DBG_LOG("WARNING: Total CPU megacycles reported as 0. Possibly invalid ferret results.\n");
                                result_data_value["cpu_fps_full_system@3GHz"] = 0.0;
                            } else {
                                result_data_value["cpu_fps_full_system@3GHz"] = static_cast<double>(frames) / full_system_cpu_runtime_3ghz;
                            }

                            if (duration < 5.0) {
                                DBG_LOG("WARNING: Runtime was less than 5 seconds, this can lead to inaccurate CPU load measurements (increasing runtime is highly recommended)\n");
                            }

                            DBG_LOG("CPU full system FPS@3GHz = %f, CPU main thread FPS@3GHz = %f\n", result_data_value["cpu_fps_full_system@3GHz"].asDouble(), result_data_value["cpu_fps_main_thread@3GHz"].asDouble());
                        } else {
                            DBG_LOG("Ferret data has no postprocessed results! Possibly wrong input parameters (check frequency list and cpu list in input json)\n");
                        }
                    }
                }
            } catch (...) {
                DBG_LOG("ERROR: Exception raised during CPU FPS calculations. Potentially broken JSON value returned by collector module.\n");
            }
        }

        if (!mProgramAttributeListMap.empty())
        {
            Json::Value programs;
            for (ProgramAttributeListMap_t::iterator it = mProgramAttributeListMap.begin(); it != mProgramAttributeListMap.end(); ++it)
            {
                std::stringstream programIdSS;
                programIdSS << it->first;
                int attrIndex = 0;
                for (AttributeList_t::iterator lit = it->second.begin(); lit != it->second.end(); ++lit)
                {
                    programs[programIdSS.str()][attrIndex] = *lit;
                    ++attrIndex;
                }
            }
            result_data_value["programs_with_unused_active_attributes"] = programs;
        }

        if (!mProgramInfoList.empty())
        {
            Json::Value programs;
            int i = 0;
            for (ProgramInfoList_t::iterator it = mProgramInfoList.begin(); it != mProgramInfoList.end(); ++it, ++i)
            {
                Json::Value programInfoJson = *it;
                programs[i] = programInfoJson;
            }
            result_data_value["programInfos"] = programs;
        }

        // Get chosen EGL configuration information
        Json::Value fb_config;
        if (gRetracer.mOptions.mForceOffscreen)
        {
            fb_config["msaaSamples"] = gRetracer.mOptions.mOffscreenConfig.msaa_samples;
            fb_config["colorBitsRed"] = gRetracer.mOptions.mOffscreenConfig.red;
            fb_config["colorBitsGreen"] = gRetracer.mOptions.mOffscreenConfig.green;
            fb_config["colorBitsBlue"] = gRetracer.mOptions.mOffscreenConfig.blue;
            fb_config["colorBitsAlpha"] = gRetracer.mOptions.mOffscreenConfig.alpha;
            fb_config["depthBits"] = gRetracer.mOptions.mOffscreenConfig.depth;
            fb_config["stencilBits"] = gRetracer.mOptions.mOffscreenConfig.stencil;
        }
        else
        {
            EglConfigInfo info = GLWS::instance().getSelectedEglConfig();
            fb_config["msaaSamples"] = info.msaa_sample_buffers == 1 && info.msaa_samples > 0 ? info.msaa_samples : 0;
            fb_config["colorBitsRed"] = info.red;
            fb_config["colorBitsGreen"] = info.green;
            fb_config["colorBitsBlue"] = info.blue;
            fb_config["colorBitsAlpha"] = info.alpha;
            fb_config["depthBits"] = info.depth;
            fb_config["stencilBits"] = info.stencil;
        }
        result_data_value["fb_config"] = fb_config;

        // Add to result list
        result_list_value.append(result_data_value);
        result_value["result"] = result_list_value;
    }

    Json::StyledWriter writer;
    std::string data = writer.write(result_value);

#ifdef ANDROID
    std::string outputfile = "/sdcard/results.json";
#else
    std::string outputfile = "results.json";
#endif
    if (!mResultFile.empty())
    {
        outputfile = mResultFile;
    }
    FILE* fp = fopen(outputfile.c_str(), "w");
    if (!fp)
    {
        DBG_LOG("Failed to open output JSON %s: %s\n", outputfile.c_str(), strerror(errno));
        return false;
    }
    size_t written;
    int err = 0;
    do
    {
        clearerr(fp);
        written = fwrite(data.c_str(), data.size(), 1, fp);
        err = ferror(fp);
    } while (!written && (err == EAGAIN || err == EWOULDBLOCK || err == EINTR));
    if (err)
    {
        DBG_LOG("Failed to write output JSON: %s\n", strerror(err));
    }
    fsync(fileno(fp));
    fclose(fp);
    DBG_LOG("Results written to %s\n", outputfile.c_str());
    return true;
}
