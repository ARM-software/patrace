// Swiss army knife tool for patrace - for all kinds of misc stuff

#include <utility>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <limits.h>
#include <unordered_map>

#include "tool/parse_interface.h"

#include "common/in_file.hpp"
#include "common/file_format.hpp"
#include "common/api_info.hpp"
#include "common/parse_api.hpp"
#include "common/trace_model.hpp"
#include "common/gl_utility.hpp"
#include "common/os.hpp"
#include "eglstate/context.hpp"
#include "tool/config.hpp"
#include "base/base.hpp"
#include "tool/utils.hpp"

#define DEBUG_LOG(...) if (debug) DBG_LOG(__VA_ARGS__)

static bool onlycount = false;
static bool verbose = false;
static int endframe = -1;
static bool waitsync = false;

static void printHelp()
{
    std::cout <<
        "Usage : converter [OPTIONS] trace_file.pat new_file.pat\n"
        "Options:\n"
        "  --waitsync    Add a non-zero timeout to glClientWaitSync and glWaitSync calls\n"
        "  --end FRAME   End frame (terminates trace here)\n"
        "  --verbose     Print more information while running\n"
        "  -c            Only count and report instances, do no changes\n"
        "  -h            Print help\n"
        "  -v            Print version\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

static void writeout(common::OutFile &outputFile, common::CallTM *call)
{
    if (onlycount) return;
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];
    char *dest = buffer;
    dest = call->Serialize(dest);
    outputFile.Write(buffer, dest-buffer);
}

int converter(ParseInterface& input, common::OutFile& outputFile)
{
    common::CallTM *call = nullptr;
    int count = 0;

    // Go through entire trace file
    while ((call = input.next_call()))
    {
        if (call->mCallName == "glClientWaitSync")
        {
            const uint64_t timeout = call->mArgs[2]->GetAsUInt64();
            if (timeout == 0)
            {
                call->mArgs[2]->mUint64 = UINT64_MAX;
                count++;
            }
            writeout(outputFile, call);
        }
        else
        {
            writeout(outputFile, call);
        }
    }
    printf("Calls changed: %d\n", count);
    return count;
}

int main(int argc, char **argv)
{
    int argIndex = 1;
    for (; argIndex < argc; ++argIndex)
    {
        std::string arg = argv[argIndex];

        if (arg[0] != '-')
        {
            break;
        }
        else if (arg == "-h")
        {
            printHelp();
            return 1;
        }
        else if (arg == "--end")
        {
            argIndex++;
            endframe = atoi(argv[argIndex]);
        }
        else if (arg == "--verbose")
        {
            verbose = true;
        }
        else if (arg == "--waitsync")
        {
            waitsync = true;
        }
        else if (arg == "-c")
        {
            onlycount = true;
        }
        else if (arg == "-v")
        {
            printVersion();
            return 0;
        }
        else
        {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            printHelp();
            return 1;
        }
    }

    if ((argIndex + 2 > argc && !onlycount) || (onlycount && argIndex + 1 > argc))
    {
        printHelp();
        return 1;
    }
    std::string source_trace_filename = argv[argIndex++];
    ParseInterface inputFile(true);
    inputFile.setQuickMode(true);
    inputFile.setScreenshots(false);
    if (!inputFile.open(source_trace_filename))
    {
        std::cerr << "Failed to open for reading: " << source_trace_filename << std::endl;
        return 1;
    }

    Json::Value header = inputFile.header;
    common::OutFile outputFile;
    if (!onlycount)
    {
        std::string target_trace_filename = argv[argIndex++];
        if (!outputFile.Open(target_trace_filename.c_str()))
        {
            std::cerr << "Failed to open for writing: " << target_trace_filename << std::endl;
            return 1;
        }
    }
    int count = converter(inputFile, outputFile);
    if (!onlycount)
    {
        Json::Value info;
        if (waitsync) info["waitsync"] = count;
        addConversionEntry(header, "converter", source_trace_filename, info);
        Json::FastWriter writer;
        const std::string json_header = writer.write(header);
        outputFile.mHeader.jsonLength = json_header.size();
        outputFile.WriteHeader(json_header.c_str(), json_header.size());
    }
    inputFile.close();
    outputFile.Close();
    return 0;
}
