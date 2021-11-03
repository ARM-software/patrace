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
        "Usage : rename_call Options\n"
        "Options:\n"
        "  -r <function to rename> <function to rename it to>, rename the function. It's exclusive with -s option.\n"
        "  -s <singlesurface>, specify singlesurface. It will rename all swapbuffers on other surfaces to glFlush call.\n"
        "  -f <source trace> <target trace>, required option.\n"
        "  -h print help\n"
        "  -v print version\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

static common::FrameTM* _curFrame = NULL;
static unsigned _curFrameIndex = 0;
static unsigned _curCallIndexInFrame = 0;

static void writeout(common::OutFile &outputFile, common::CallTM *call, bool injected)
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

static int readValidValue(char* v)
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

int main(int argc, char **argv)
{
    int argIndex = 1;
    int surfaceIdx = -1;
    std::string orig, dest;
    char* source_trace_filename = NULL;
    char* target_trace_filename = NULL;

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
        else if (!strcmp(arg, "-s"))
        {
            surfaceIdx = readValidValue(argv[++argIndex]);
            orig = "eglSwapBuffersxxx";
            dest = "glFlush";
        }
        else if (!strcmp(arg, "-r"))
        {
            orig = argv[++argIndex];
            dest = argv[++argIndex];
        }
        else if (!strcmp(arg, "-f"))
        {
            source_trace_filename = argv[++argIndex];
            target_trace_filename = argv[++argIndex];
        }
        else
        {
            printf("Error: Unknow option %s\n", arg);
            printHelp();
            return 1;
        }
    }
    if (source_trace_filename == NULL || target_trace_filename == NULL)
    {
        printf("source_trace_file and target_trace_file is required.\n");
        printHelp();
        return 1;
    }

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
    if (surfaceIdx != -1)
    {
        header["singleSurface"] = surfaceIdx;
    }
    info["renamed_from"] = orig;
    info["renamed_to"] = dest;
    addConversionEntry(header, "rename_call", source_trace_filename, info);
    Json::FastWriter writer;
    const std::string json_header = writer.write(header);
    outputFile.mHeader.jsonLength = json_header.size();
    outputFile.WriteHeader(json_header.c_str(), json_header.size());

    common::CallTM *call = NULL;
    int renamed = 0;

    int surfcnt = 0;
    int surface = 0;
    bool injected = false;
    while ((call = next_call(inputFile)))
    {
        injected = false;
        if (surfaceIdx != -1)
        {
            if (call->mCallName == "eglCreateWindowSurface" || call->mCallName == "eglCreateWindowSurface2")
            {
                if (surfaceIdx == surfcnt)
                {
                    //get surface from call
                    surface = call->mRet.GetAsInt();
                }
                surfcnt++;
            }
            else if (call->mCallName == "eglSwapBuffers" || call->mCallName == "eglSwapBuffersWithDamageKHR")
            {
                int swap_surf = call->mArgs[1]->GetAsInt();
                if (swap_surf != surface)  // replacing swapbuffers with glFlush when specifying singlesurface
                {
                    call->mCallId = common::gApiInfo.NameToId(dest.c_str());
                    call->mCallName = dest;
                    call->mRet.Reset();
                    call->ClearArguments();
                    injected = true;
                    renamed++;
                }
            }
        }
        else if (call->mCallName == orig)
        {
            call->mCallId = common::gApiInfo.NameToId(dest.c_str());
            call->mCallName = dest;
            renamed++;
        }
        writeout(outputFile, call, injected);
    }

    if (surfaceIdx != -1 && surface == 0)    DBG_LOG("WARNING: The given singlesurface does not match any surfaces.\n");
    DBG_LOG("Renamed %d calls %s to %s\n", renamed, orig.c_str(), dest.c_str());
    inputFile.Close();
    outputFile.Close();

    return 0;
}
