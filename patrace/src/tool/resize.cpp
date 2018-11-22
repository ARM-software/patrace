#include <vector>
#include <list>
#include <map>
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


static void printHelp()
{
    std::cout <<
        "Usage : resize <new width> <new height> <source trace> <target trace>\n"
        "Options:\n"
        "  -h            print help\n"
        "  -v            print version\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

static common::FrameTM* _curFrame = NULL;
static unsigned _curFrameIndex = 0;
static unsigned _curCallIndexInFrame = 0;

static void writeout(common::OutFile &outputFile, common::CallTM *call)
{
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];
    char *dest = buffer;
    dest = call->Serialize(dest);
    outputFile.Write(buffer, dest-buffer);
}

static common::CallTM* next_call(common::TraceFileTM &_fileTM)
{
    if (_curCallIndexInFrame >= _curFrame->GetLoadedCallCount())
    {
        _curFrameIndex++;
        if (_curFrameIndex >= _fileTM.mFrames.size())
            return NULL;
        if (_curFrame) _curFrame->UnloadCalls();
        _curFrame = _fileTM.mFrames[_curFrameIndex];
        _curFrame->LoadCalls(_fileTM.mpInFileRA);
        _curCallIndexInFrame = 0;
    }
    common::CallTM *call = _curFrame->mCalls[_curCallIndexInFrame];
    _curCallIndexInFrame++;
    return call;
}

int main(int argc, char **argv)
{
    int argIndex = 1;
    for (; argIndex < argc; ++argIndex)
    {
        const char *arg = argv[argIndex];

        if (arg[0] != '-')
            break;

        if (!strcmp(arg, "-h"))
        {
            printHelp();
            return 1;
        }
        else if (!strcmp(arg, "-v"))
        {
            printVersion();
            return 1;
        }
        else
        {
            printf("Error: Unknown option %s\n", arg);
            printHelp();
            return 1;
        }
    }

    if (argIndex + 4 > argc)
    {
        printHelp();
        return 1;
    }
    const int overrideResWidth = atoi(argv[argIndex++]);
    const int overrideResHeight = atoi(argv[argIndex++]);

    const char* source_trace_filename = argv[argIndex++];
    const char* target_trace_filename = argv[argIndex++];

    common::TraceFileTM inputFile;
    common::gApiInfo.RegisterEntries(common::parse_callbacks);
    if (!inputFile.Open(source_trace_filename))
    {
        PAT_DEBUG_LOG("Failed to open for reading: %s\n", source_trace_filename);
        return 1;
    }
    _curFrame = inputFile.mFrames[0];
    _curFrame->LoadCalls(inputFile.mpInFileRA);

    common::OutFile outputFile;
    if (!outputFile.Open(target_trace_filename))
    {
        PAT_DEBUG_LOG("Failed to open for writing: %s\n", target_trace_filename);
        return 1;
    }

    Json::Value header = inputFile.mpInFileRA->getJSONHeader();

    Json::Value resizeInfo;
    resizeInfo["width"] = overrideResWidth;
    resizeInfo["height"] = overrideResHeight;
    addConversionEntry(header, "resize", source_trace_filename, resizeInfo);

    Json::Value threadArray = header["threads"];
    unsigned numThreads = threadArray.size();
    if (numThreads == 0)
    {
        DBG_LOG("Bad number of threads: %d\n", numThreads);
        return 1;
    }
    for (unsigned i = 0; i < numThreads; i++)
    {
        unsigned tid = threadArray[i]["id"].asUInt();
        unsigned resW = threadArray[i]["winW"].asUInt();
        unsigned resH = threadArray[i]["winH"].asUInt();

        threadArray[i]["winW"] = overrideResWidth;
        threadArray[i]["winH"] = overrideResHeight;
        DBG_LOG("Trace header resolution override for thread[%d] from %d x %d -> %d x %d\n",
                tid, resW, resH, overrideResWidth, overrideResHeight);
    }
    header["threads"] = threadArray;

    Json::FastWriter writer;
    const std::string json_header = writer.write(header);
    outputFile.mHeader.jsonLength = json_header.size();
    outputFile.WriteHeader(json_header.c_str(), json_header.size());

    common::CallTM *call = NULL;
    while ((call = next_call(inputFile)))
    {
        if (call->mCallName == "glViewport")
        {
            GLsizei w = call->mArgs[2]->GetAsInt();
            GLsizei h = call->mArgs[3]->GetAsInt();

            call->ClearArguments();
            call->mArgs.push_back(new common::ValueTM(0));
            call->mArgs.push_back(new common::ValueTM(0));
            call->mArgs.push_back(new common::ValueTM(overrideResWidth));
            call->mArgs.push_back(new common::ValueTM(overrideResHeight));
            DBG_LOG("Viewport was resized from %d x %d to %d x %d\n", w, h, overrideResWidth, overrideResHeight);
        }

        writeout(outputFile, call);
    }

    inputFile.Close();
    outputFile.Close();

    return 0;
}
