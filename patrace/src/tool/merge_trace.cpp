#include <vector>
#include <list>
#include <map>
#include <set>
#include <string>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "common/in_file.hpp"
#include "common/file_format.hpp"
#include "common/out_file.hpp"
#include "common/api_info.hpp"
#include "common/parse_api.hpp"
#include "common/trace_model.hpp"
#include "common/os.hpp"
#include "eglstate/context.hpp"
#include "tool/config.hpp"
#include "base/base.hpp"
#include "tool/utils.hpp"
#include "json/writer.h"
#include "json/reader.h"

static void printHelp()
{
    std::cout <<
        "Usage : merge_trace Options\n"
        "Options:\n"
        "  -i <input trace file 1> <input trace file 2>...<input trace file n>, input the trace files wanted to be merged. required option.\n"
        "  -o <ouput merged trace file>, output the merged trace file. required option.\n"
        "  -h print help\n"
        "  -v print version\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

struct frame_info_t
{
    frame_info_t() : curFrameIndex(0), curCallIndexInFrame(0) {}

    common::FrameTM *curFrame = NULL;
    unsigned curFrameIndex;
    unsigned curCallIndexInFrame;
};


static void writeout(common::OutFile &outputFile, common::CallTM *call, bool injected)
{
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];
    char *dest = buffer;
    dest = call->Serialize(dest, -1, injected);
    outputFile.Write(buffer, dest-buffer);
}

static common::CallTM* next_call(common::TraceFileTM &_fileTM, struct frame_info_t &frame_info)
{
    if (frame_info.curCallIndexInFrame >= frame_info.curFrame->GetLoadedCallCount())
    {
        frame_info.curFrameIndex++;
        if (frame_info.curFrameIndex >= _fileTM.mFrames.size())
            return NULL;
        if (frame_info.curFrame) frame_info.curFrame->UnloadCalls();
        frame_info.curFrame = _fileTM.mFrames[frame_info.curFrameIndex];
        frame_info.curFrame->LoadCalls(_fileTM.mpInFileRA);
        frame_info.curCallIndexInFrame = 0;
    }
    common::CallTM *call = frame_info.curFrame->mCalls[frame_info.curCallIndexInFrame];
    frame_info.curCallIndexInFrame++;
    return call;
}

static void addJsonArrayEntry(Json::Value &rootArray, Json::Value &nextArray)
{
    for (Json::ArrayIndex i = 0; i < nextArray.size(); i++)
    {
        Json::Value value = nextArray[i];
        rootArray.append(value);
    }
}

static void parseCreateCalls(std::map<unsigned int, std::pair<int, unsigned int>> &resMap, std::map<std::pair<int, unsigned int>, unsigned int> &resMapRev, common::CallTM *early_call, int early_trace_index)
{
    unsigned int new_name = early_call->mRet.GetAsUInt();
    unsigned int old_name = new_name;
    while (resMap.count(new_name)!=0)
    {
        new_name = new_name+1;
    }
    resMap.insert({new_name, {early_trace_index, old_name}});
    resMapRev.insert({{early_trace_index, old_name}, new_name});
    early_call->mRet.SetAsUInt(new_name);
}

static void parseNormalCalls(std::map<std::pair<int, unsigned int>, unsigned int> &resMapRev, common::CallTM *early_call, int early_trace_index, int n)
{
    unsigned int old_name = early_call->mArgs[n]->GetAsUInt();
    early_call->mArgs[n]->SetAsUInt(resMapRev[{early_trace_index, old_name}]);
}

static void clearMaps(std::map<unsigned int, std::pair<int, unsigned int>> &resMap, std::map<std::pair<int, unsigned int>, unsigned int> &resMapRev, int early_trace_index, unsigned int new_name)
{
    resMapRev.erase({early_trace_index, resMap[new_name].second});
    resMap.erase(new_name);
}

int main(int argc, char **argv)
{
    int argIndex = 1;
    std::vector<std::string> source_trace_filename;
    std::string target_trace_filename;

    for (; argIndex < argc; ++argIndex)
    {
        const char *arg = argv[argIndex];

        if (!strcmp(arg, "-h"))
        {
            printHelp();
            return 1;
        }
        else if (!strcmp(arg, "-v"))
        {
            printVersion();
            return 0;
        }
        else if (!strcmp(arg, "-i"))
        {
            while ( (argIndex+1)<argc && *(argv[(argIndex+1)])!='-' )
            {
                source_trace_filename.emplace_back(argv[++argIndex]);
            }
        }
        else if (!strcmp(arg, "-o"))
        {
            target_trace_filename= argv[++argIndex];
        }
        else if (!strcmp(arg, "-t"))
        {
            //to do
        }
        else
        {
            printf("Error: Unknow option %s\n", arg);
            printHelp();
            return 1;
        }
    }

    uint32_t source_trace_count = source_trace_filename.size();

    if (source_trace_count == 0 || target_trace_filename.c_str() == NULL)
    {
        printf("source_trace_file and target_trace_file is required.\n");
        printHelp();
        return 1;
    }
    if (source_trace_count == 1)
    {
        printf("only 1 source trace file, it does not need to merge.\n");
        return 1;
    }

    // input files open
    std::vector<common::TraceFileTM*> inputFiles(source_trace_count);
    std::vector<frame_info_t> curFrames(source_trace_count);
    std::map<int, common::CallTM*> callMap;
    std::map<std::pair<int, int>, int> threadIdMap; // <<trace_index, tid>, new_tid>
    std::map<unsigned int, std::pair<int, unsigned int>> surfaceMap; // Have a surfaceMap <surface_name, <trace_index, pre_name>> and a rev map
    std::map<std::pair<int, unsigned int>, unsigned int> surfaceMapRev;
    std::map<unsigned int, std::pair<int, unsigned int>> contextMap;
    std::map<std::pair<int, unsigned int>, unsigned int> contextMapRev;
    std::map<unsigned int, std::pair<int, unsigned int>> imageMap;
    std::map<std::pair<int, unsigned int>, unsigned int> imageMapRev;
    std::map<unsigned int, std::pair<int, unsigned int>> syncMap;
    std::map<std::pair<int, unsigned int>, unsigned int> syncMapRev;

    for (uint32_t i=0; i<source_trace_count; i++)
    {
        common::TraceFileTM *inputFile = new common::TraceFileTM;
        if ( !inputFile->Open(source_trace_filename[i].c_str()) )
        {
            PAT_DEBUG_LOG("Failed to open for reading: %s\n", source_trace_filename[i].c_str());
            return 1;
        }
        inputFiles[i] = inputFile;
        struct frame_info_t frame;
        frame.curFrame = inputFile->mFrames[0];
        frame.curFrame->LoadCalls(inputFile->mpInFileRA);
        curFrames[i] = frame;

        callMap[i] = next_call(*inputFiles[i], curFrames[i]);
    }

    common::gApiInfo.RegisterEntries(common::parse_callbacks);

    // output file
    common::OutFile outputFile;
    if (!outputFile.Open(target_trace_filename.c_str()))
    {
        PAT_DEBUG_LOG("Failed to open for writing: %s\n", target_trace_filename.c_str());
        return 1;
    }

    // merge calls according to timestamp label
    common::CallTM *early_call = NULL;
    int early_trace_index = 0;
    int call_count = 0;

    std::map<int, common::CallTM*>::iterator early_it = callMap.begin();
    std::map<int, common::CallTM*>::iterator it = early_it;
    std::vector<unsigned long long> timestampSet(source_trace_count, 0);
    while (callMap.size() > 1)
    {
        early_it = callMap.begin();
        early_trace_index = early_it->first;
        early_call = early_it->second;
        it = early_it;

        if (early_call->mCallName == "paTimestamp")
        {
            timestampSet[early_trace_index] = early_call->mArgs[0]->GetAsUInt64();
            callMap[early_trace_index] = next_call(*inputFiles[early_trace_index], curFrames[early_trace_index]);
            early_call = early_it->second;
        }
        for (++it; it != callMap.end(); it++)
        {
            if (it->second->mCallName == "paTimestamp")
            {
                timestampSet[it->first] = it->second->mArgs[0]->GetAsUInt64();
                callMap[it->first] = next_call(*inputFiles[it->first], curFrames[it->first]);
            }
            if (timestampSet[early_trace_index] > timestampSet[it->first])
            {
                early_trace_index = it->first;
                early_call = it->second;
            }
        }

        // Then conlict of resources:Drawable(Surface), Context, Image, Sync
        if (early_call->mCallName == "eglCreateWindowSurface2" || early_call->mCallName == "eglCreatePbufferSurface")
        {
            parseCreateCalls(surfaceMap, surfaceMapRev, early_call, early_trace_index);
            // Here is to set win, its value doesn't matter, just need to make sure they are different.
            early_call->mArgs[2]->SetAsUInt(early_call->mRet.GetAsUInt());
        }
        if (early_call->mCallName == "eglQuerySurface" || early_call->mCallName == "eglSwapBuffers")
        {
            parseNormalCalls(surfaceMapRev, early_call, early_trace_index, 1);
        }
        if (early_call->mCallName == "eglCreateContext")
        {
            parseCreateCalls(contextMap, contextMapRev, early_call, early_trace_index);
        }
        if (early_call->mCallName == "eglCreateImageKHR" || early_call->mCallName == "eglCreateImage")
        {
            parseCreateCalls(imageMap, imageMapRev, early_call, early_trace_index);
            parseNormalCalls(contextMapRev, early_call, early_trace_index, 1);
        }
        if (early_call->mCallName == "glEGLImageTargetTexture2DOES")
        {
            parseNormalCalls(imageMapRev, early_call, early_trace_index, 1);
        }
        if (early_call->mCallName == "eglDestroyImageKHR" || early_call->mCallName == "eglDestroyImage")
        {
            parseNormalCalls(imageMapRev, early_call, early_trace_index, 1);
            clearMaps(imageMap, imageMapRev, early_trace_index, early_call->mArgs[1]->GetAsUInt());
        }
        if (early_call->mCallName == "eglCreateSyncKHR")
        {
            parseCreateCalls(syncMap, syncMapRev, early_call, early_trace_index);
        }
        if (early_call->mCallName == "eglDestroySyncKHR")
        {
            parseNormalCalls(syncMapRev, early_call, early_trace_index, 1);
            clearMaps(syncMap, syncMapRev, early_trace_index, early_call->mArgs[1]->GetAsUInt());
        }
        if (early_call->mCallName == "eglDestroyContext")
        {
            parseNormalCalls(contextMapRev, early_call, early_trace_index, 1);
            clearMaps(contextMap, contextMapRev, early_trace_index, early_call->mArgs[1]->GetAsUInt());
        }
        if (early_call->mCallName == "eglDestroySurface")
        {
            parseNormalCalls(surfaceMapRev, early_call, early_trace_index, 1);
            clearMaps(surfaceMap, surfaceMapRev, early_trace_index, early_call->mArgs[1]->GetAsUInt());
        }
        if (early_call->mCallName == "eglGetCurrentContext")
        {
            unsigned int context_name = early_call->mRet.GetAsUInt();
            early_call->mRet.SetAsUInt(contextMapRev[{early_trace_index, context_name}]);
        }
        if (early_call->mCallName == "eglMakeCurrent")
        {
            parseNormalCalls(surfaceMapRev, early_call, early_trace_index, 1);
            parseNormalCalls(surfaceMapRev, early_call, early_trace_index, 2);
            parseNormalCalls(contextMapRev, early_call, early_trace_index, 3);
        }
        if (threadIdMap.count({early_trace_index, early_call->mTid}) == 0) threadIdMap.insert({{early_trace_index, early_call->mTid}, threadIdMap.size()});
        early_call->mTid = threadIdMap[{early_trace_index, early_call->mTid}];

        writeout(outputFile, early_call, false);
        call_count++;
        if ( (callMap[early_trace_index] = next_call(*inputFiles[early_trace_index], curFrames[early_trace_index])) == NULL )
        {
            PAT_DEBUG_LOG("trace index %d ends.\n", early_trace_index);
            callMap.erase(early_trace_index);
        }
    }

    // the remaining last one trace
    early_it = callMap.begin();
    early_trace_index = early_it->first;
    while ( (early_call = next_call(*inputFiles[early_trace_index], curFrames[early_trace_index])) )
    {
        if (early_call->mCallName == "paTimestamp")
        {
            timestampSet[early_trace_index] = early_call->mArgs[0]->GetAsUInt64();
            callMap[early_trace_index] = next_call(*inputFiles[early_trace_index], curFrames[early_trace_index]);
            early_call = early_it->second;
        }
        if (threadIdMap.count({early_trace_index, early_call->mTid}) == 0) threadIdMap.insert({{early_trace_index, early_call->mTid}, threadIdMap.size()});
        early_call->mTid = threadIdMap[{early_trace_index, early_call->mTid}];
        writeout(outputFile, early_call, false);
        call_count++;
    }

    callMap.erase(early_trace_index);

    // merge json header
    // TODO: After merging, it would update index item for context, surfaces, tid for thread in json header.
    Json::Value jsonRoot = inputFiles[0]->mpInFileRA->getJSONHeader();
    jsonRoot["timestamping"] = false;
    for (uint32_t i=1; i<source_trace_count; i++)
    {
        Json::Value next = inputFiles[i]->mpInFileRA->getJSONHeader();
        jsonRoot["callCnt"] = call_count;
        jsonRoot["frameCnt"] = jsonRoot["frameCnt"].asInt() + next["frameCnt"].asInt();
        jsonRoot["glesVersion"] = (jsonRoot["glesVersion"].asInt()>next["glesVersion"].asInt())?jsonRoot["glesVersion"].asInt():next["glesVersion"].asInt();   // here, the glesVersion in json header is set and required from API. save the max?

        jsonRoot["trace_date"] = jsonRoot["trace_date"].asString() + ", " + next["trace_date"].asString();
        jsonRoot["tracer"] = jsonRoot["tracer"].asString() + ", " + next["tracer"].asString();
        jsonRoot["tracer_extensions"] = jsonRoot.get("tracer_extensions", "").asString() + ", " + next.get("tracer_extensions", "").asString();

        jsonRoot["capture_device_renderer"] = jsonRoot["capture_device_renderer"].asString() + ", " + next["capture_device_renderer"].asString();
        jsonRoot["capture_device_shading_language_version"] = jsonRoot["capture_device_shading_language_version"].asString() + ", " + next["capture_device_shading_language_version"].asString();
        jsonRoot["capture_device_vendor"] = jsonRoot["capture_device_vendor"].asString() + ", " + next["capture_device_vendor"].asString();
        jsonRoot["capture_device_version"] = jsonRoot["capture_device_version"].asString() + ", " + next["capture_device_version"].asString();

        // Merge extension lists
        std::set<std::string> extensionsSet;
        std::string extension;
        std::string delimiter = " ";
        std::string s = jsonRoot["capture_device_extensions"].asString();
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        // create set for jsonRoot
        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
        {
            extension = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            extensionsSet.insert(extension);
        }
        pos_start = 0;
        pos_end = delim_len = delimiter.length();
        s = next["capture_device_extensions"].asString();
        // insert&append new extentions
        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
        {
            extension = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            if (extensionsSet.count(extension) == 0)
            {
                extensionsSet.insert(extension);
                jsonRoot["capture_device_extensions"] = jsonRoot["capture_device_extensions"].asString() + extension + delimiter;
            }
        }

        addJsonArrayEntry(jsonRoot["contexts"], next["contexts"]);
        addJsonArrayEntry(jsonRoot["surfaces"], next["surfaces"]);
        addJsonArrayEntry(jsonRoot["texCompress"], next["texCompress"]);
        addJsonArrayEntry(jsonRoot["threads"], next["threads"]);
    }
    Json::Value info;
    info["command_type"] = "merge trace";
    addConversionEntry2(jsonRoot, "merge_trace", source_trace_filename, info);

    Json::FastWriter writer;
    const std::string json_header = writer.write(jsonRoot);
    outputFile.mHeader.jsonLength = json_header.size();
    outputFile.WriteHeader(json_header.c_str(), json_header.size());

    // close and free resources
    for (uint32_t i=0; i<inputFiles.size(); i++)
        inputFiles[i]->Close();
    outputFile.Close();
    inputFiles.clear();
    curFrames.clear();

    return 0;
}