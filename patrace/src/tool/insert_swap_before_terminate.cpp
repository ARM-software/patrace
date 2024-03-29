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
        "Usage : insert_swap_before_terminate <source trace> <target trace>\n"
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

static void writeout(common::OutFile &outputFile, common::CallTM *call, bool injected = false)
{
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];
    char *dest = buffer;
    dest = call->Serialize(dest, -1, injected);
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
            return 0;
        }
        else
        {
            printf("Error: Unknow option %s\n", arg);
            printHelp();
            return 1;
        }
    }

    if (argIndex + 2 > argc)
    {
        printHelp();
        return 1;
    }
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
    Json::Value info;
    addConversionEntry(header, "insert_swap_before_terminate", source_trace_filename, info);
    Json::FastWriter writer;
    const std::string json_header = writer.write(header);
    outputFile.mHeader.jsonLength = json_header.size();
    outputFile.WriteHeader(json_header.c_str(), json_header.size());
    common::CallTM *call = NULL;
    int display = 0;
    int surface = 0;
    while ((call = next_call(inputFile)))
    {
        if (call->mCallName == "eglMakeCurrent")
        {
            int new_display = call->mArgs[0]->GetAsInt();
            int new_surface = call->mArgs[1]->GetAsInt();
            int tid = call->mTid;
            if (new_surface == 0)
            {
                common::CallTM swap("eglSwapBuffers");
                swap.mArgs.push_back(new common::ValueTM(display));
                swap.mArgs.push_back(new common::ValueTM(surface));
                swap.mRet = common::ValueTM((int)EGL_TRUE);
                swap.mTid = tid;
                writeout(outputFile, &swap, true);
                printf("injected eglSwapBuffers(%d, %d) into thread %d\n", display, surface, tid);
            }
            display = new_display;
            surface = new_surface;
            printf("[%d] eglMakeCurrent(%d, %d, ...)\n", tid, display, surface);
        }
        writeout(outputFile, call);
    }
    inputFile.Close();
    outputFile.Close();
    return 0;
}
