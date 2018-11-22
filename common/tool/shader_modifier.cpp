#include <vector>
#include <GLES2/gl2.h>

#include "eglstate/context.hpp"
#include "tool/trace_interface.hpp"
#include "tool/config.hpp"
#include "base/base.hpp"
using namespace pat;

namespace
{

void printHelp()
{
    std::cout <<
        "Usage : shader_modifier [OPTIONS] source_trace target_trace\n"
        "Options:\n"
        "  -h            print help\n"
        "  -v            print version\n"
        "  -hfp          change float precision of shaders from medium to high\n"
        "  -replace str1 str2\n"
        "                replace all occurrence of str1 by str2 in shaders\n"
        ;
}

void printVersion()
{
    std::cout << "Version: " PATRACE_VERSION << std::endl;
}

// Replace fromStr to toStr for every string in sources, and combine them into result
void replace(const std::vector<std::string> &sources, const std::string &fromStr, const std::string &toStr, std::string &result)
{
    const unsigned int n = sources.size();
    result.clear();
    for (unsigned int i = 0; i < n; ++i)
    {
        std::string buffer = sources[i];
        size_t found = buffer.find(fromStr);
        while (found != std::string::npos)
        {
            buffer.replace(found, fromStr.size(), toStr);
            found = buffer.find(fromStr, found + toStr.size());
        }
        result += buffer;
    }
}

}

extern "C"
int main(int argc, char **argv)
{
    std::string source_str, dest_str;

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
        else if (!strcmp(arg, "-hfp"))
        {
            source_str = std::string("precision mediump float;");
            dest_str = std::string("#ifdef GL_FRAGMENT_PRECISION_HIGH\n    precision highp float;\n#else\n    precision mediump float;\n#endif\n");
        }
        else if (!strcmp(arg, "-replace"))
        {
            source_str = argv[++argIndex];
            dest_str = argv[++argIndex];
        }
        else
        {
            printf("Error: Unknow option %s\n", arg);
            printHelp();
            return 1;
        }
    }

    if (source_str.empty() || dest_str.empty())
    {
        printHelp();
        return 1;
    }

    if (argIndex + 2 > argc)
    {
        printHelp();
        return 1;
    }
    const char * source_trace_filename = argv[argIndex++];
    const char * target_trace_filename = argv[argIndex++];

    std::shared_ptr<InputFileInterface> inputFile(GenerateInputFile());
    if (!inputFile->open(source_trace_filename))
    {
        PAT_DEBUG_LOG("Error : failed to open %s\n", source_trace_filename);
        return 1;
    }

    std::shared_ptr<OutputFileInterface> outputFile(GenerateOutputFile());
    if (!outputFile->open(target_trace_filename))
    {
        PAT_DEBUG_LOG("Error : failed to open %s\n", target_trace_filename);
        return 1;
    }

    // copy the json string of the header from the input file to the output file
    const std::string json_header = inputFile->json_header();
    outputFile->write_json_header(json_header);

    CallInterface *call = NULL;
    while ((call = inputFile->next_call()))
    {
        if (strcmp(call->GetName(), "glShaderSource") == 0)
        {
            const GLuint callNo = call->GetNumber();
            const GLuint n = call->arg_to_uint(1);
            std::vector<std::string> sources;

            for (GLuint i = 0; i < n; i++)
            {
                const char *str = call->array_arg_to_str(2, i);
                if (call->arg_to_bool(3))
                {
                    const GLuint len = call->array_arg_to_uint(3, i);
                    sources.push_back(std::string(str, len));
                }
                else
                {
                    sources.push_back(std::string(str));
                }
            }

            std::string result;
            replace(sources, source_str, dest_str, result);

            call->clear_args(1);
            call->push_back_sint_arg(1);
            call->push_back_array_arg(1);
            call->set_array_item(2, 0, result);
            call->push_back_null_arg();

            printf("LOG : Process call no.%d\n", callNo);
        }

        outputFile->write(call);
        delete call;
    }

    inputFile->close();
    outputFile->close();

    return 0;
}
