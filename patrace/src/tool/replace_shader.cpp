#include <map>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

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
        "Usage : replace_shader [OPTIONS] <source trace> <callNo> <target trace>\n"
        "\n"
        "<callNo> must refer to a glShaderSource-call. The shader at that call\n"
        "will be replaced to shader.txt in the current working directory\n"
        "\n"
        "Options:\n"
        "  -h     print help\n"
        "  -v     print version\n"
        "  -d     dump existing shader to shader.txt in CWD\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

static void writeout(common::OutFile &outputFile, common::CallTM *call)
{
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];

    char *dest = call->Serialize(buffer);
    outputFile.Write(buffer, dest-buffer);
}

int main(int argc, char **argv)
{
    bool dump = false;

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
        else if (!strcmp(arg, "-d"))
        {
            dump = true;
        }
        else
        {
            printf("Error: Unknow option %s\n", arg);
            printHelp();
            return 1;
        }
    }

    if (argIndex + 3 > argc)
    {
        printHelp();
        return 1;
    }

    std::string source_trace_filename = argv[argIndex++];
    std::string callNoStr = argv[argIndex++];
    unsigned callNo = std::stoi(callNoStr);
    std::string target_trace_filename = argv[argIndex++];

    common::gApiInfo.RegisterEntries(common::parse_callbacks);

    // Load input trace
    common::TraceFileTM inputFile;
    if (!inputFile.Open(source_trace_filename.c_str()))
    {
        DBG_LOG("Failed to open for reading: %s\n", source_trace_filename.c_str());
        return 1;
    }

    // Load output trace
    common::OutFile tmpFile;
    if (!tmpFile.Open(target_trace_filename.c_str()))
    {
        PAT_DEBUG_LOG("Failed to open for writing: %s\n", target_trace_filename.c_str());
        return 1;
    }

    // Iterate over all calls
    common::CallTM *call = NULL;
    while ((call = inputFile.NextCall()))
    {
        if (call->mCallNo == callNo)
        {
            if (call->mCallName != "glShaderSource")  {
                DBG_LOG("Error: CallNo wasn't a call to glShaderSource\n");
                exit(1);
            }
            if (dump) {
                FILE *fp = fopen("shader.txt", "w");
                if (!fp)
                {
                    std::cerr << "Failed to open output file: " << strerror(errno) << std::endl;
                    exit(1);
                }
                for (unsigned i = 0; i < call->mArgs[2]->mArrayLen; i++)
                {
                    std::string source = call->mArgs[2]->mArray[i].GetAsString();
                    fwrite(source.c_str(), source.size(), 1, fp);
                }
                fclose(fp);
                exit(0);
            } else {
                std::ifstream t("shader.txt");
                std::stringstream buffer;
                buffer << t.rdbuf();

                call->mArgs[2]->mArrayLen = 1;
                call->mArgs[2]->mArray[0].SetAsString(buffer.str());
                call->mArgs[3]->mArrayLen = 0;
                DBG_LOG("Replaced shader!\n");
            }
        }

        writeout(tmpFile, call);
    }

    // Write header
    Json::Value header = inputFile.mpInFileRA->getJSONHeader();
    Json::Value info;
    addConversionEntry(header, "replace_shader", source_trace_filename, info);

    Json::FastWriter writer;
    const std::string json_header = writer.write(header);
    tmpFile.mHeader.jsonLength = json_header.size();
    tmpFile.WriteHeader(json_header.c_str(), json_header.size());

    inputFile.Close();
    tmpFile.Close();

    return 0;
}
